import ctypes
import numpy as np
import os
import platform

class AIModel:
    """Interface Python pour la librairie C++ de l'IA (Architecture Buffer)."""
    def __init__(self):
        # On charge la librairie selon l'OS
        if platform.system() == "Darwin": lib_name = "libai_lib.dylib"
        elif platform.system() == "Windows": lib_name = "ai_lib.dll"
        else: lib_name = "libai_lib.so"
        
        current_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(current_dir)
        lib_path = os.path.join(project_root, "ai_engine", "build", lib_name)
        
        try:
            self.lib = ctypes.CDLL(lib_path)
            print(f"Librairie C++ chargée : {lib_path}")
        except OSError as e:
            print(f"ERREUR FATALE : Impossible de charger {lib_path}. {e}")
            raise

        # On définit la signature C++ (Buffer)
        # Signature : int* get_best_move_buffer(int* input_buffer, int depth, int mode, int* out_size)
        
        self.lib.get_best_move_buffer.argtypes = [
            np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags='C_CONTIGUOUS'), # input_buffer
            ctypes.c_int,                                                         # depth
            ctypes.c_int,                                                         # mode
            ctypes.POINTER(ctypes.c_int)                                          # out_size (pointeur)
        ]
        # Le retour est un pointeur vers un tableau d'entiers (int*)
        self.lib.get_best_move_buffer.restype = ctypes.POINTER(ctypes.c_int)

    def decode_move(self, result_list: list) -> dict:
        """Traduit la liste d'entiers renvoyée par le C++ en dictionnaire."""
        if not result_list:
            return {"col": 0, "kill": None} # Sécurité

        col = result_list[0]
        move = {"col": col, "kill": None}
        
        # Si la liste contient 3 éléments (ex: [-1, 5, 2]), c'est une destruction
        if len(result_list) >= 3:
            move["kill"] = (result_list[1], result_list[2])
            
        return move

    def get_best_move(self, board, depth, mode, p1_stock=0, p2_stock=0) -> dict:
        """
        Prépare le buffer (Plateau + Stocks), appelle le C++, et traduit la réponse.
        """
        # On construit le Buffer d'Entrée (42 cases + 2 Stocks = 44 entiers)
        flat_board = board.flatten().astype(np.int32)
        
        input_buffer = np.zeros(44, dtype=np.int32)
        input_buffer[:42] = flat_board
        # Ajout du contexte (Stocks) à la fin du buffer
        input_buffer[42] = p1_stock
        input_buffer[43] = p2_stock
        
        # On prépare le pointeur de taille de sortie
        out_size = ctypes.c_int(0)
        
        # Appel C++
        result_ptr = self.lib.get_best_move_buffer(input_buffer, depth, mode, ctypes.byref(out_size))
        
        # On lit le résultat (Conversion Pointeur C -> Liste Python)
        result_list = [result_ptr[i] for i in range(out_size.value)]
        
        return self.decode_move(result_list)
