"""
analyze_phantoms.py
Lists the phantom types (impulse volumes of the physics model, A12) of every object tag of a map, from the definition dumps of the
DefinitionsInspector (Dump for field-usage report): phmo, mode, hlmt and the object groups bloc, scen, mach, ctrl, eqip, vehi.
For each phantom type: flags (who it ignores), size range, direction / axis / center / orbit acceleration and maximum velocity,
and its marker resolved in the render model (node, position and +X direction in object space). Ends with the lift detection rule
(H1 of Ignore/Tareas/Transversal/A12_Lifts.txt) checked against the tag catalog (Data/Tags/ObjectTags.xml).

    python Tools/analyze_phantoms.py [DUMP_FOLDER]      (default: Ignore/Temporal)
"""
import json, os, re, sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
D = (sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "Ignore", "Temporal")).rstrip("/\\") + os.sep
CATALOG = os.path.join(REPO, "Data", "Tags", "ObjectTags.xml")
def load(g):
    out={}
    for l in open(D+f'definitions_dump_{g}.ndjson',encoding='utf-8'):
        j=json.loads(l); out[j['tag']]=j['fields']
    return out
phmo,mode,hlmt=load('phmo'),load('mode'),load('hlmt')
objs={}
for g in ['bloc','scen','mach','ctrl','eqip','vehi']:
    for t,f in load(g).items(): objs[t]=(g,f.get('Base.ModelTagName',''))
cat={}
for m in re.finditer(r'<tag name="([^"]+)" group="(\w+)"[^>]*function="(\w+)" subtype="(\w*)"',open(CATALOG,encoding='utf-8').read()):
    cat[m[1]]=(m[3],m[4])
def arr(f,prefix,fields):
    res=[];i=0
    while f.get(f'{prefix}[{i}].{fields[0]}') is not None:
        res.append({k:f.get(f'{prefix}[{i}].{k}') for k in fields}); i+=1
    return res
PT=['Flags','MinimumSize','MaximumSize','MarkerName','AlignmentMarkerName','DirectionAcceleration','DirectionMaximumVelocity','AxisAcceleration','AxisMaximumVelocity','CenterAcceleration','CenterMaximumVelocity','OrbitAcceleration','OrbitMaximumVelocity','HookesLawE','LinearDeadRadius']
FL={0:'fx',1:'force',2:'nograv',3:'-players',4:'-nonplayers',5:'-bipeds',6:'-vehicles',7:'-weapons',8:'-equip',9:'-term',10:'-proj',11:'-scen',12:'-mach',13:'-ctrl',14:'-ssce',15:'-crates',16:'-creat',17:'-giants',18:'-efsc',24:'localphys',25:'nolindamp',26:'noangdamp',27:'-dead',30:'nolatch'}
def flags(v):
    v=int(v); return ' '.join(FL.get(b,f'b{b}') for b in range(32) if v>>b&1)
def qrot(x,y,z,w,v):
    # rotate v by quaternion (x,y,z,w)
    qx,qy,qz,qw=x,y,z,w; vx,vy,vz=v
    tx=2*(qy*vz-qz*vy); ty=2*(qz*vx-qx*vz); tz=2*(qx*vy-qy*vx)
    return (vx+qw*tx+(qy*tz-qz*ty), vy+qw*ty+(qz*tx-qx*tz), vz+qw*tz+(qx*ty-qy*tx))
def markers(modename,nameid):
    f=mode.get(modename)
    if not f: return None
    i=0;res=[]
    while f.get(f'MarkerGroups[{i}].NameId') is not None:
        if f[f'MarkerGroups[{i}].NameId']==nameid:
            for m in arr(f,f'MarkerGroups[{i}].Markers',['NodeIndex','Translation.X','Translation.Y','Translation.Z','Rotation.X','Rotation.Y','Rotation.Z','Rotation.W']):
                q=[float(m[k]) for k in ['Rotation.X','Rotation.Y','Rotation.Z','Rotation.W']]
                fwd=qrot(*q,(1,0,0))
                res.append((m['NodeIndex'],tuple(round(float(m[k]),2) for k in ['Translation.X','Translation.Y','Translation.Z']),tuple(round(c,2) for c in fwd)))
        i+=1
    return res
rows=[]
for tag,(g,model) in sorted(objs.items()):
    h=hlmt.get(model)
    if not h: continue
    pm=h.get('PhysicsModelTagName',''); rm=h.get('RenderModelTagName','')
    p=phmo.get(pm)
    if not p: continue
    pts=arr(p,'PhantomTypes',PT)
    if not pts: continue
    # which phantom types are used by phantom shapes / materials
    shapes=[]; b=0
    while p.get(f'RigidBodies[{b}].NodeIndex') is not None:
        shapes+= [dict(s,body=b) for s in arr(p,f'RigidBodies[{b}].Shapes',['Type','IsPhantom','PhantomTypeIndex','MaterialIndex','CollisionGroup'])]; b+=1
    mats=arr(p,'Materials',['PhantomTypeIndex','CollisionGroup'])
    solid=sum(1 for s in shapes if s['IsPhantom']=='false')
    ph=sum(1 for s in shapes if s['IsPhantom']=='true')
    fn=cat.get(tag,('?',''))
    print(f"\n{g} {tag}  [{fn[0]}/{fn[1]}]  shapes solid {solid} phantom {ph}")
    for i,t in enumerate(pts):
        mk=int(t['MarkerName']); ms=markers(rm,str(mk)) if mk else None
        print(f"   type {i}: {flags(t['Flags'])}  size {t['MinimumSize']}-{t['MaximumSize']}  dir {float(t['DirectionAcceleration']):g}/{float(t['DirectionMaximumVelocity']):g}  axis {float(t['AxisAcceleration']):g}/{float(t['AxisMaximumVelocity']):g}  center {float(t['CenterAcceleration']):g}/{float(t['CenterMaximumVelocity']):g}  orbit {float(t['OrbitAcceleration']):g}  hooke {float(t['HookesLawE']):g}")
        print(f"           marker {mk:#x} -> {ms if mk else 'EMPTY (object up)'}")

print("\n==== H1: lift para jugadores = algun phantom type que NO ignora players (bit 3) ni bipeds (bit 5), AxisAcceleration > 0, DirectionMaximumVelocity >= 2")
for tag,(g,model) in sorted(objs.items()):
    h=hlmt.get(model); p=phmo.get(h.get('PhysicsModelTagName','')) if h else None
    if not p: continue
    pts=arr(p,'PhantomTypes',PT)
    def ok(t):
        f=int(t['Flags']); return not (f>>3&1) and not (f>>5&1) and float(t['AxisAcceleration'])>0 and float(t['DirectionMaximumVelocity'])>=2
    det=any(ok(t) for t in pts); veh=any(ok(t) and not (int(t['Flags'])>>6&1) for t in pts)
    items=any(not (int(t['Flags'])>>7&1) and float(t['DirectionMaximumVelocity'])>0 for t in pts)
    fn=cat.get(tag,('?',''))[0]
    if det or fn=='Lift':
        print(f"  {'OK ' if det==(fn=='Lift') else 'BAD'} detect={det} catalog={fn:<8} vehicles={veh} weapons={items}  {tag}")
