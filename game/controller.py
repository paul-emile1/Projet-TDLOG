from game.gamemanager import variantes, InvalidMove, GameManager
from game.graphicinterface import Interface
from typing import Optional
import numpy as np
from game.computer import Computer


class Controller:
    def __init__(self):
        self._interface : Interface = Interface() # Interface graphique
        self._variantes : list[type[GameManager]] = variantes # Variantes de jeu disponibles, c'est une liste de classes
        self._GameManager : Optional[GameManager] = None # Instance de la variante choisie
        self._in_menu : bool = True # Indique si on est dans le menu principal
        self._in_menu_players : bool = False # Indique si on est dans le menu de choix du nombre de joueurs
        self._in_menu_difficulty : bool = False # Indique si on est dans le menu de choix de la difficulté
        self._in_game : bool = False # Indique si on est dans la boucle de jeu

    def start(self):
        while self._interface._running:
            if self._in_menu:
                self.menu_principal()
            elif self._in_menu_players:
                self.menu_players()
            elif self._in_menu_difficulty:
                self.menu_difficulty()
            elif self._in_game:
                self.game_loop()

    def menu_principal(self):
        choice = self._interface.send_menu(
            "Bienvenue sur Puissance 4",
            [v.name for v in self._variantes]
        )

        if choice is not None:
            self._GameManager = self._variantes[choice]()
            self._in_menu = False
            self._in_menu_players = True

    def menu_players(self):
        choice = self._interface.send_menu(
            "Choisissez le nombre de joueurs",
            ["1 Joueur", "2 Joueurs"]
        )

        if choice is not None:
            if choice == 0:
                self._GameManager.nb_players = 1
                self._in_menu_players = False
                self._in_menu_difficulty = True
            else:
                self._GameManager.nb_players = 2
                self._in_menu_players = False
                self._in_game = True

    def menu_difficulty(self):
        choice = self._interface.send_menu(
            "Choisissez la difficulté",
            [c.name for c in self._GameManager.computer_difficulties]
        )

        if choice is not None:
            self._GameManager.current_computer_difficulty = self._GameManager.computer_difficulties[choice]()
            self._in_menu_difficulty = False
            self._in_game = True

    def game_loop(self):
        # 1. Demande coup (Attend un clic)
        move = self._interface.send_game(
            self._GameManager.current_player,
            self._GameManager.board
        )

        if move is None:  # Retour Menu
            self._in_game = False
            self._in_menu = True
            self._GameManager = None
            return

        try:
            # 2. Joue le coup
            self._GameManager.play(move)

            # 3. Vérifie l'état du jeu
            if self._GameManager.victory:
                # Affiche le plateau final SANS attendre (plus de latence)
                self._interface.refresh_only(self._GameManager.current_player, self._GameManager.board)

                # Notifie le gagnant
                self._interface.notify_victory(self._GameManager.current_player)

                self._in_game = False
                self._in_menu = True

            elif self._GameManager.draw:
                self._interface.refresh_only(self._GameManager.current_player, self._GameManager.board)
                self._interface.notify_draw()
                self._in_game = False
                self._in_menu = True

            elif self._GameManager.event:
                self._interface.notify_message(self._GameManager.message_event)
                self._interface.refresh_only(self._GameManager.current_player, self._GameManager.board)


        except InvalidMove:
            pass  # Recommence