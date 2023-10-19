g++ lib/engine/*.cpp lib/glad/wgl.c lib/glad/gl.c *.cpp -g -I ./include/ -lopengl32 -lgdi32 -lpthread -lwsock32 -o main.exe && main.exe
