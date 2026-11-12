"""Training lists (LS.2, D285, D286): the catalog of the maps and the lists of match setups an agent trains on."""
from .catalog import MAPS_PATH, MapInfo, load_catalog
from .lists import (ListError, StaleListError, TrainingEntry, TrainingList, load_list, save_list)
