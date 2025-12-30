from typing import Optional
from abc import ABC, abstractmethod
import numpy as np

class Computer(ABC):


    @abstractmethod
    def make_move(self, board: np.ndarray) -> tuple[int, int]:
        pass





classicgame_computers : list[type[Computer]] = []
variante_1_computers : list[type[Computer]] = []