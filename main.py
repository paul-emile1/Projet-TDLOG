import sys

if __name__ == "__main__":
    try:
        from game.controller import Controller
        app_ctrl = Controller()
        app_ctrl.start()
    except KeyboardInterrupt:
        sys.exit(0)