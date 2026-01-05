cmake_minimum_required(VERSION 3.10)
project(Puissance4_AI)

# Standard C++ (C++17 est un bon compromis moderne)
set(CMAKE_CXX_STANDARD 17)

# Dossier des fichiers .h
include_directories(include)

# 1. Création de la librairie (C'est ça que le Python utilisera)
# On crée une librairie partagée (SHARED)
add_library(ai_lib SHARED src/ai_core.cpp)

# 2. Création de l'exécutable de test (C'est ça que TU utiliseras pour tes tests)
# Il contient ton main de test + la logique de l'IA
add_executable(run_ai_test src/main.cpp src/ai_core.cpp)