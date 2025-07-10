# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
# CXX: nome da variável padrão que representa o compilador C++. Aqui é o g++.
# CXXFLAGS: são as opções de compilação:
# -std=c++20: usar a versão C++20.
# -Wall: mostra todas as warnings (melhor prática!).
# -Iinclude: diz ao compilador que os arquivos .hpp estão na pasta include/.

# Arquivos-fonte
SRC = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
# SRC: lista todos os arquivos .cpp na pasta src/ e subpastas.
OBJ = $(SRC:.cpp=.o)
# OBJ: converte cada .cpp em .o (objeto) usando substituição de sufixo.
TARGET = orderbook

# Regra principal
all: $(TARGET)
# Essa é a primeira regra, e por padrão, é a que o Make executa se você digitar só make.
# Aqui diz: para construir tudo (all), eu preciso do $(TARGET).

# Linkagem
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@
# $(TARGET) é orderbook (definido lá em cima).
# Depende dos $(OBJ) (arquivos .o).
# O comando compila todos os .o e gera o binário orderbook.
# O $@ representa o nome da "meta" atual (neste caso, orderbook)

# Compilar cada .cpp em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
# Regra genérica: transforma um .cpp em um .o.
# % é um coringa (wildcard) que casa nomes como main.cpp → main.o.
# $< é o arquivo de entrada (por exemplo, main.cpp).
# $@ é o arquivo de saída (main.o).
# Essa regra é chamada automaticamente para cada arquivo .cpp listado em SRC.

# Limpeza
clean:
	rm -f $(OBJ) $(TARGET)
# Essa regra limpa tudo que foi gerado: .o e o binário final.
# make clean executa isso.

.PHONY: all clean
# make tenta interpretar "targets" como arquivos.
# PHONY diz que all e clean são só comandos, não arquivos reais.
