CC=mpic++
CFLAGS=-c
EXECUTABLE=run

all: createData $(EXECUTABLE)

createData: dataGen/createData.cpp
	$(CC) dataGen/createData.cpp -o createData

clean:
	rm *.o */*.o */*/*.o run createData

$(EXECUTABLE): main.o Objects/Triangle.o IO/ReadData.o Rendering/Renderer_Example.o Rendering/OpenGL_Example.o Composition/Composition_Example.o Rendering/OpenGL_common/shader.o Composition/IceT_Example.o
	$(CC) -L/usr/lib -L./Composition/IceT-2-1-1/build/lib  main.o Objects/Triangle.o Objects/Vertex.o IO/ReadData.o Rendering/Renderer_Example.o Rendering/OpenGL_Example.o Rendering/OpenGL_common/shader.o Composition/Composition_Example.o Composition/IceT_Example.o -o $(EXECUTABLE) -lglfw -lGL -lGLU -lglut -lGLEW -lIceTCore -lIceTGL -lIceTMPI


main.o: main.cpp Objects/Vertex.o Objects/Triangle.o IO/ReadData.o Rendering/Renderer_Example.o Rendering/OpenGL_Example.o Composition/Composition_Example.o Composition/IceT_Example.o
	$(CC) $(CFLAGS) -I./Composition/IceT-2-1-1/src/include -I./Composition/IceT-2-1-1/build/src/include main.cpp -o main.o

Objects/Vertex.o: Objects/Vertex.cpp Objects/Vertex.h
	$(CC) $(CFLAGS) Objects/Vertex.cpp -o Objects/Vertex.o

Objects/Triangle.o: Objects/Triangle.cpp Objects/Triangle.h Objects/Vertex.o 
	$(CC) $(CFLAGS) Objects/Triangle.cpp -o Objects/Triangle.o

IO/ReadData.o: IO/ReadData.cpp IO/ReadData.h Objects/Triangle.o Objects/Vertex.o 
	$(CC) $(CFLAGS) IO/ReadData.cpp -o IO/ReadData.o

Rendering/Renderer_Example.o: Rendering/Renderer_Example.cpp Rendering/Renderer_Example.h Objects/Triangle.o
	$(CC) $(CFLAGS) Rendering/Renderer_Example.cpp -o Rendering/Renderer_Example.o

Composition/Composition_Example.o: Composition/Composition_Example.cpp Composition/Composition_Example.h Objects/Triangle.o
	$(CC) $(CFLAGS) Composition/Composition_Example.cpp -o Composition/Composition_Example.o

Rendering/OpenGL_common/shader.o: Rendering/OpenGL_common/shader.cpp Rendering/OpenGL_common/shader.hpp
	$(CC) $(CFLAGS) -I/usr/include Rendering/OpenGL_common/shader.cpp -o Rendering/OpenGL_common/shader.o 

Rendering/OpenGL_Example.o: Rendering/OpenGL_Example.cpp Rendering/OpenGL_Example.h Rendering/OpenGL_common/shader.o Objects/Triangle.o
	$(CC) $(CFLAGS) -I/usr/include Rendering/OpenGL_Example.cpp -o Rendering/OpenGL_Example.o 

Composition/IceT_Example.o: Composition/IceT_Example.cpp Composition/IceT_Example.h Objects/Triangle.o
	$(CC) $(CFLAGS) -I./Composition/IceT-2-1-1/src/include -I./Composition/IceT-2-1-1/build/src/include Composition/IceT_Example.cpp -o Composition/IceT_Example.o

