NAME 		= webserv

CXX 		= c++
CXXFLAGS 	= -std=c++98 -Wall -Werror -Wextra -MD -MP -g

OBJ_DIR		= ./objs
SRCS_DIR	= ./srcs

SRCS		= 	$(addprefix $(SRCS_DIR)/,\
				main.cpp \
				Socket.cpp \
				Exception.cpp)
				# Config.cpp \

OBJS		= $(SRCS:$(SRCS_DIR)%.cpp=$(OBJ_DIR)/%.o)
DEPS		 = $(SRCS:$(SRCS_DIR)%.cpp=${OBJ_DIR}/%.d)


all: 	$(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean:
	rm -rf $(OBJ_DIR)
	rm -f $(NAME)

re:	fclean all

-include $(DEPS)

.PHONY: all clean fclean re
