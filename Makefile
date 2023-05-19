CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = ircserv
SRC = srcs/main.cpp \
	  srcs/Client.cpp \
	  srcs/Server.cpp \
	  srcs/Channel.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
