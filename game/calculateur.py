import ctypes
import numpy as np
import os
import sys

class AIModel:
    def __init__(self):
        # 1. Trouver le chemin du fichier compilé (.dylib sur Mac, .so sur Linux, .dll sur Windows)
        # On suppose que le fichier est dans ai_engine/build/
        # Ajuste le chemin si besoin !
        
        current_dir = os.path.dirname(os.path.abspath(__file__))
        lib_path = os.path.join(current_dir, "ai_engine/build/libai_lib.dylib") 
        
        if not os.path.exists(lib_path):
             lib_path = os.path.join(current_dir, "ai_engine/build/Debug/libai_lib.dylib")

        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"Impossible de trouver le moteur IA à : {lib_path}. As-tu bien compilé avec CMake ?")

        print(f" Moteur IA chargé depuis : {lib_path}")

        # 2. Charger la librairie
        self.lib = ctypes.CDLL(lib_path)

        # 3. Définir la signature de la fonction C++
        # int get_best_move(int* board, int depth)
        
        # Argument 1 : Un pointeur vers un tableau d'entiers (le board)
        self.lib.get_best_move.argtypes = [
            np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags='C_CONTIGUOUS'),
            ctypes.c_int # depth
        ]
        
        # Type de retour : int (la colonne)
        self.lib.get_best_move.restype = ctypes.c_int

    def get_best_move(self, board_numpy, depth=4):
        """
        board_numpy: Un tableau numpy 6x7 (ou plat de 42).
                     IMPORTANT : Doit contenir des int32.
                     Convention : 0=Vide, 1=IA, -1=Adversaire (selon tes constantes C++)
        """
        # S'assurer que le tableau est plat (1D) et en int32 (C++ n'aime pas le int64 par défaut de python)
        board_flat = board_numpy.flatten().astype(np.int32)
        
        # Appel de la fonction C++
        col = self.lib.get_best_move(board_flat, depth)
        
        return col