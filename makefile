TARGET 	= simulador
OBJ_DIR = ./obj
INC_DIR = ./include
SRC_DIR = ./src

OBJS 	= \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/sistemas.o \
	$(OBJ_DIR)/funcAux.o \

CFLAGS	= -I$(INC_DIR)

$(TARGET) : $(OBJS)
	gcc -pthread $(CFLAGS) $(OBJS) -o $(TARGET)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	gcc -pthread -c -MD $(CFLAGS) $< -o $@

-include $(OBJ_DIR)/*.d

.PHONY: clean
clean :
	@rm -r $(OBJ_DIR)
	