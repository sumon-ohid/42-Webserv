NAME 		= webserv

CXX 		= c++
CXXFLAGS 	= -std=c++98 -Wall -Werror -Wextra -MD -MP -g

OBJ_DIR		= ./objs
SRCS_DIR	= ./srcs

SRCS		= 	$(addprefix $(SRCS_DIR),\
				main.cpp \
				Socket.cpp \
				Client.cpp \
				Server.cpp \
				Epoll.cpp \
				Helper.cpp \
				Request.cpp \
				Method.cpp \
     			GetMethod.cpp \
				Response.cpp \
				Config.cpp \
				LocationConfig.cpp \
				ServerConfig.cpp \
				HandleCgi.cpp \
				ServerManager.cpp \
				ErrorHandle.cpp \
				PostMethod.cpp \
				DeleteMethod.cpp \
				)

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

valgrind:
	valgrind ./$(NAME)

.PHONY: all clean fclean re
