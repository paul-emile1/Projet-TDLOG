import unittest
import numpy as np
from game.gamemanager import ClassicGame, Variante_1, InvalidMove

class TestClassicGame(unittest.TestCase):
    
    def setUp(self):
        """Initialisation avant chaque test"""
        self.game = ClassicGame()
        self.game._current_player = 1 

    def test_victoire_verticale(self):
        """Vérifie qu'aligner 4 pions verticalement donne la victoire"""
        # On met 3 pions rouges (1) dans la colonne 0
        self.game.board[5, 0] = 1
        self.game.board[4, 0] = 1
        self.game.board[3, 0] = 1
        
        # Le joueur joue le 4ème pion
        self.game.play((0, 0)) 

        self.assertTrue(self.game.victory, "La victoire devrait être validée")
        # Le gagnant reste le current_player car le jeu s'arrête
        self.assertEqual(self.game.current_player, 1, "Le gagnant (1) devrait rester le current_player")

    def test_colonne_pleine(self):
        """Vérifie qu'on ne peut pas jouer dans une colonne pleine"""
        # On remplit la colonne 0 entièrement
        self.game.board[:, 0] = 1 
        
        # On essaie de jouer encore dans la colonne 0 : Doit lever une erreur
        with self.assertRaises(InvalidMove):
            self.game.play((0, 0))


class TestVariante1(unittest.TestCase):
    
    def setUp(self):
        self.game = Variante_1()
        self.game._current_player = 1

    def test_declenchement_event_3_alignes(self):
        """
        Vérifie que si j'aligne 3 pions, je ne gagne pas tout de suite,
        mais je déclenche l'événement (pouvoir).
        """
        # Préparation : 2 pions rouges alignés horizontalement en bas
        self.game.board[5, 0] = 1
        self.game.board[5, 1] = 1
        
        # Le joueur 1 joue le 3ème pion en colonne 2
        self.game.play((0, 2))

        # Vérifications
        self.assertFalse(self.game.victory, "Pas de victoire avec seulement 3 pions")
        self.assertTrue(self.game.event, "L'événement (pouvoir) devrait être activé")
        self.assertEqual(self.game.current_player, 1, "C'est toujours au tour du joueur 1 (action bonus)")
        self.assertIn("retirer un pion", self.game.message_event)

    def test_retrait_pion_adverse_et_gravite(self):
        """
        Vérifie le retrait d'un pion adverse et la chute des pions au-dessus.
        """
        #  MISE EN SITUATION 
        # On force l'état "Event actif" pour le joueur 1
        self.game._event = True
        self.game._current_player = 1

        # On place des pions adverses (Joueur -1 / Jaune)
        # Colonne 0 : Jaune en bas, Jaune au-dessus
        self.game.board[5, 0] = -1 
        self.game.board[4, 0] = -1
        
        #  ACTION 
        # Le joueur 1 décide de retirer le pion du bas (5, 0)
        self.game.play((5, 0))

        #  VÉRIFICATIONS 
        
        # 1. Le pion du bas a dû disparaître (remplacé par celui du dessus)
        self.assertEqual(self.game.board[5, 0], -1, "Le pion du dessus aurait dû tomber")
        
        # 2. La case du dessus [4, 0] doit être vide (0) maintenant
        self.assertEqual(self.game.board[4, 0], 0, "La case libérée par la chute doit être vide")

        # 3. L'événement est fini
        self.assertFalse(self.game.event, "L'événement devrait être terminé")
        
        # 4. C'est au tour de l'adversaire (-1)
        self.assertEqual(self.game.current_player, -1, "La main doit passer à l'adversaire")

    def test_retrait_interdit_propre_pion(self):
        """On ne doit pas pouvoir retirer son propre pion"""
        self.game._event = True
        self.game._current_player = 1
        self.game.board[5, 0] = 1 # Pion rouge

        with self.assertRaises(InvalidMove):
            self.game.play((5, 0)) # Essaie de retirer son propre pion

if __name__ == '__main__':
    unittest.main()
