from game.gamemanager import variantes, InvalidMove
from game.graphicinterface import Interface


class Controller:
    """Contrôleur principal gérant la boucle de jeu et les interactions utilisateur."""
    def __init__(self):
        self._interface = Interface()
        self._variantes = variantes
        self._gestionnaire = None
        self._in_menu = True
        self._in_game = False

    def start(self):
        """Lance l'application."""
        while self._interface._running:
            if self._in_menu:
                self.menu_principal()
            elif self._in_game:
                self.game_loop()

    def menu_principal(self):
        # On affiche le choix de la Variante
        choix_variante = self._interface.send_menu(
            "Bienvenue sur Puissance 4",
            [v.name for v in self._variantes]
        )

        if choix_variante is None: 
            return # Fermeture de la fenêtre

        # On affiche le choix du Mode
        choix_mode = self._interface.send_menu(
            "Choisissez le mode de jeu",
            ["1 Joueur (Contre l'IA)", "2 Joueurs (Local)"]
        )

        if choix_mode is None: 
            return

        mode_solo = (choix_mode == 0)
        difficulty = 4

        # On demande la difficulté pour le mode solo
        if mode_solo:
            choix_diff = self._interface.send_menu(
                "Niveau de difficulté",
                ["Facile", "Moyen", "Difficile", "Expert"]
            )
            if choix_diff is None: return
            
            # Mapping : Index -> Profondeur de recherche
            niveaux = [2, 4, 6, 8]
            difficulty = niveaux[choix_diff]

        # Initialisation du gestionnaire
        self._gestionnaire = self._variantes[choix_variante](
            mode_solo=mode_solo, 
            difficulty=difficulty
        )
        
        self._in_menu = False
        self._in_game = True

    def _get_display_infos(self):
        """
        Récupère les infos contextuelles (Stocks, etc.) auprès du GameManager
        de manière robuste, quelle que soit la variante.
        """
        if not self._gestionnaire:
            return None, None
        
        p1_info = self._gestionnaire.get_info_status(-1) # Humain/Rouge
        p2_info = self._gestionnaire.get_info_status(1)  # IA/Jaune
        return p1_info, p2_info

    def game_loop(self):
        """Boucle principale d'une partie."""
        
        # On récupère les infos (ex: Stock) avant d'afficher
        info_p1, info_p2 = self._get_display_infos()

        move = self._interface.send_game(
            self._gestionnaire.current_player,
            self._gestionnaire.board,
            p1_info=info_p1,
            p2_info=info_p2
        )

        if move is None:
            self._in_game = False
            self._in_menu = True
            self._gestionnaire = None
            return

        try:
            # Gestion du tour humain
            self._gestionnaire.play(move)

            if self.check_game_end():
                return 

            # On vérifie les événements bloquants (ex: Variante 1 Bonus immédiat)
            if getattr(self._gestionnaire, 'event', False):
                return
            
            # Gestion du tour IA
            if getattr(self._gestionnaire, 'mode_solo', False):
                
                info_p1, info_p2 = self._get_display_infos()
                self._interface.refresh_only(
                    self._gestionnaire.current_player, 
                    self._gestionnaire.board, 
                    p1_info=info_p1, 
                    p2_info=info_p2
                )
                
                self._interface.pause(700) 
                
                self._gestionnaire.play_ai_turn()

                if self.check_game_end():
                    return

        except InvalidMove:
            pass

    def check_game_end(self):
        """Vérifie les conditions de fin de partie ou les événements."""
        
        info_p1, info_p2 = self._get_display_infos()

        if self._gestionnaire.victory:
            self._interface.refresh_only(
                self._gestionnaire.current_player, 
                self._gestionnaire.board,
                p1_info=info_p1, 
                p2_info=info_p2
            )
            self._interface.notify_victory(self._gestionnaire.current_player)
            self._in_game = False
            self._in_menu = True
            return True

        elif self._gestionnaire.draw:
            self._interface.refresh_only(
                self._gestionnaire.current_player, 
                self._gestionnaire.board,
                p1_info=info_p1, 
                p2_info=info_p2
            )
            self._interface.notify_draw()
            self._in_game = False
            self._in_menu = True
            return True

        # On gère les messages liés aux événements de variante
        elif getattr(self._gestionnaire, 'event', False):
            self._interface.notify_message(self._gestionnaire.message_event)
            self._interface.refresh_only(
                self._gestionnaire.current_player, 
                self._gestionnaire.board,
                p1_info=info_p1, 
                p2_info=info_p2
            )
            return False 

        return False
