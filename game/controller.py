from gamemanager import variantes, InvalidMove
from graphicinterface import Interface


class Controller:
    def __init__(self):
        self._interface = Interface()
        self._variantes = variantes
        self._gestionnaire = None
        self._in_menu = True
        self._in_game = False

    def start(self):
        while self._interface._running:
            if self._in_menu:
                self.menu_principal()
            elif self._in_game:
                self.game_loop()

    def menu_principal(self):
        choix = self._interface.send_menu(
            "Bienvenue sur Puissance 4",
            [v.name for v in self._variantes]
        )

        if choix is not None:
            self._gestionnaire = self._variantes[choix]()
            self._in_menu = False
            self._in_game = True

    def game_loop(self):
        # 1. Demande coup (Attend un clic)
        move = self._interface.send_game(
            self._gestionnaire.current_player,
            self._gestionnaire.board
        )

        if move is None:  # Retour Menu
            self._in_game = False
            self._in_menu = True
            self._gestionnaire = None
            return

        try:
            # 2. Joue le coup
            self._gestionnaire.play(move)

            # 3. VÃ©rifie Ã©tat
            if self._gestionnaire.victory:
                # Affiche le plateau final SANS attendre (plus de latence)
                self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)

                # Notifie le gagnant
                self._interface.notify_victory(self._gestionnaire.current_player)

                self._in_game = False
                self._in_menu = True

            elif self._gestionnaire.draw:
                self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)
                self._interface.notify_draw()
                self._in_game = False
                self._in_menu = True

            elif self._gestionnaire.event:
                self._interface.notify_message(self._gestionnaire.message_event)
                self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)


        except InvalidMove:
            pass  # Recommence