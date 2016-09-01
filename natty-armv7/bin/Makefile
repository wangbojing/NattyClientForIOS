


CLIENT_OBJ = NattyNetwork.o NattyRBTree.o NattyAbstractClass.o NattyTimer.o NattyProtoClient.o \
	NattyUtils.o NattyLetter.o 

BIN_OBJ = NattyClientMain.o

CLINET_LIB = libnatty-client.a
CLIENT_BIN = NattyClient

#$(SERVER_BIN) : $(SERVER_OBJ)
#	$(CC) -o $(SERVER_BIN) $(SERVER_OBJ) $(FLAG)

#$(CLIENT_BIN) : $(CLIENT_OBJ)
#	$(CC) -o $(CLIENT_BIN) $(CLIENT_OBJ) $(FLAG)

bin : 
	$(AR) $(ROOT_DIR)/$(CLINET_LIB) $(CLIENT_OBJ)
	$(CC) $(BIN_OBJ) -L../ -lnatty-client -o $(ROOT_DIR)/$(CLIENT_BIN) $(FLAG) -g

