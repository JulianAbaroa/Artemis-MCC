"""
ppo.py
PPO in one file, in the style of CleanRL (D147): an MLP actor-critic with one categorical head per MultiDiscrete branch, GAE,
clipped objective. Agents of an environment step together and their transitions are pooled (shared parameters). The logic ticks
are paused while the network trains (Bridge.paused, D143), so the game never waits on an update.
"""
import time
from dataclasses import dataclass

import numpy as np
import torch
from torch import nn


@dataclass
class PpoConfig:
    steps: int = 2048
    gamma: float = 0.99
    gae_lambda: float = 0.95
    learning_rate: float = 3e-4
    epochs: int = 4
    minibatch: int = 512
    clip: float = 0.2
    value_coef: float = 0.5
    entropy_coef: float = 0.01
    max_grad_norm: float = 0.5
    hidden: int = 256


def Layer(inputs, outputs, std=np.sqrt(2)):
    layer = nn.Linear(inputs, outputs)
    nn.init.orthogonal_(layer.weight, std)
    nn.init.constant_(layer.bias, 0.0)
    return layer


class Policy(nn.Module):
    def __init__(self, obs_size, nvec, hidden):
        super().__init__()
        self.nvec = list(nvec)
        self.body = nn.Sequential(Layer(obs_size, hidden), nn.Tanh(), Layer(hidden, hidden), nn.Tanh())
        self.actor = Layer(hidden, sum(self.nvec), std=0.01)
        self.critic = Layer(hidden, 1, std=1.0)

    def value(self, obs):
        return self.critic(self.body(obs)).squeeze(-1)

    def act(self, obs, actions=None):
        """Samples (or scores) the branches. Returns actions, their log probability, the entropy and the value."""
        hidden = self.body(obs)
        logits = torch.split(self.actor(hidden), self.nvec, dim=-1)
        dists = [torch.distributions.Categorical(logits=l) for l in logits]
        if actions is None:
            actions = torch.stack([d.sample() for d in dists], dim=-1)
        log_prob = torch.stack([d.log_prob(a) for d, a in zip(dists, actions.unbind(-1))], dim=-1).sum(-1)
        entropy = torch.stack([d.entropy() for d in dists], dim=-1).sum(-1)
        return actions, log_prob, entropy, self.critic(hidden).squeeze(-1)


class Ppo:
    """Collects config.steps decisions per agent from the environment, then trains on them with the ticks paused."""

    def __init__(self, env, bridge, obs_size, nvec, config: PpoConfig = None, log=print):
        self.env = env
        self.bridge = bridge
        self.config = config or PpoConfig()
        self.policy = Policy(obs_size, nvec, self.config.hidden)
        self.optimizer = torch.optim.Adam(self.policy.parameters(), lr=self.config.learning_rate, eps=1e-5)
        self.log = log
        self.observations = None
        self.episode_returns = {}
        self.finished = []
        self.counts = {}
        self.global_step = 0

    def _start_episode(self):
        self.observations, _ = self.env.reset()
        self.episode_returns = {h: 0.0 for h in self.observations}

    def collect(self):
        """Plays config.steps decisions. Returns the batch of every agent, flattened."""
        c = self.config
        if self.observations is None:
            self._start_episode()
        rows = {"obs": [], "actions": [], "log_prob": [], "values": [], "rewards": [], "dones": [], "agent": []}
        per_agent = {}
        for _ in range(c.steps):
            handles = list(self.observations)
            obs = torch.as_tensor(np.stack([self.observations[h] for h in handles]))
            with torch.no_grad():
                actions, log_prob, _, values = self.policy.act(obs)
            next_obs, rewards, terminated, truncated, infos = self.env.step(
                {h: actions[i].numpy() for i, h in enumerate(handles)})
            for i, h in enumerate(handles):
                reward = rewards[h]
                if truncated[h] and not terminated[h]:
                    # Time cut: bootstrap with the value of the state the episode was cut in.
                    with torch.no_grad():
                        reward += c.gamma * float(self.policy.value(torch.as_tensor(next_obs[h])[None])[0])
                per_agent.setdefault(h, []).append(len(rows["obs"]))
                rows["obs"].append(obs[i].numpy())
                rows["actions"].append(actions[i].numpy())
                rows["log_prob"].append(float(log_prob[i]))
                rows["values"].append(float(values[i]))
                rows["rewards"].append(reward)
                rows["dones"].append(float(terminated[h] or truncated[h]))
                rows["agent"].append(h)
                self.episode_returns[h] += rewards[h]
                for name in infos[h]["events"]:
                    self.counts[name] = self.counts.get(name, 0) + 1
            self.global_step += 1
            if any(terminated.values()) or any(truncated.values()):
                self.finished += list(self.episode_returns.values())
                self._start_episode()
            else:
                self.observations = next_obs
        # Value of the state each agent is left in, for the bootstrap of the last step.
        last_values = {}
        with torch.no_grad():
            for h, o in self.observations.items():
                last_values[h] = float(self.policy.value(torch.as_tensor(o)[None])[0])
        batch = {k: np.asarray(v) for k, v in rows.items() if k != "agent"}
        batch["advantages"] = np.zeros(len(rows["obs"]), dtype=np.float32)
        for h, indices in per_agent.items():
            gae = 0.0
            next_value = last_values.get(h, 0.0)
            for i in reversed(indices):
                non_terminal = 1.0 - batch["dones"][i]
                delta = batch["rewards"][i] + c.gamma * next_value * non_terminal - batch["values"][i]
                gae = delta + c.gamma * c.gae_lambda * non_terminal * gae
                batch["advantages"][i] = gae
                next_value = batch["values"][i]
        batch["returns"] = batch["advantages"] + batch["values"]
        return batch

    def update(self, batch):
        c = self.config
        obs = torch.as_tensor(batch["obs"], dtype=torch.float32)
        actions = torch.as_tensor(batch["actions"], dtype=torch.int64)
        old_log_prob = torch.as_tensor(batch["log_prob"], dtype=torch.float32)
        advantages = torch.as_tensor(batch["advantages"], dtype=torch.float32)
        returns = torch.as_tensor(batch["returns"], dtype=torch.float32)
        size = len(obs)
        stats = []
        for _ in range(c.epochs):
            order = torch.randperm(size)
            for start in range(0, size, c.minibatch):
                index = order[start:start + c.minibatch]
                _, log_prob, entropy, values = self.policy.act(obs[index], actions[index])
                ratio = (log_prob - old_log_prob[index]).exp()
                adv = advantages[index]
                adv = (adv - adv.mean()) / (adv.std() + 1e-8)
                policy_loss = torch.max(-adv * ratio, -adv * ratio.clamp(1 - c.clip, 1 + c.clip)).mean()
                value_loss = 0.5 * (values - returns[index]).pow(2).mean()
                loss = policy_loss + c.value_coef * value_loss - c.entropy_coef * entropy.mean()
                self.optimizer.zero_grad()
                loss.backward()
                nn.utils.clip_grad_norm_(self.policy.parameters(), c.max_grad_norm)
                self.optimizer.step()
                with torch.no_grad():
                    stats.append((float(policy_loss), float(value_loss), float(entropy.mean()),
                                  float(((ratio - 1) - (ratio.log())).mean())))
        return np.mean(stats, axis=0)

    def train(self, updates, checkpoint=None):
        for update in range(1, updates + 1):
            started = time.time()
            batch = self.collect()
            collected = time.time()
            with self.bridge.paused():
                policy_loss, value_loss, entropy, kl = self.update(batch)
                if checkpoint:
                    torch.save(self.policy.state_dict(), checkpoint)
            recent = self.finished[-20:]
            self.log(f"update {update}: step {self.global_step}, episodes {len(self.finished)}, "
                     f"return {np.mean(recent) if recent else float('nan'):+.3f}, events {self.counts}, "
                     f"collect {collected - started:.1f} s, train {time.time() - collected:.1f} s, "
                     f"loss pi {policy_loss:+.4f} v {value_loss:.4f} ent {entropy:.3f} kl {kl:.4f}")
