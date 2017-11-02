#include "OpenGL_Example.h"
#include "OpenGL_common/shader.hpp"

void OpenGL_Example::readTriangles(vector<Triangle>* tris, GLfloat* v_buffer, GLfloat* c_buffer, int* resolution) {
	for (int i = 0; i < tris->size(); i++) {
		Triangle* t = &tris->at(i);
				
		v_buffer[i*9 + 0] = 2.0f * (float)t->v1.p1 / resolution[0] - 1.0f;
		v_buffer[i*9 + 1] = 2.0f * (float)t->v1.p2 / resolution[1] - 1.0f;
		v_buffer[i*9 + 2] = 2.0f * (float)t->v1.p3 / resolution[2] - 1.0f;
		v_buffer[i*9 + 3] = 2.0f * (float)t->v2.p1 / resolution[0] - 1.0f;
		v_buffer[i*9 + 4] = 2.0f * (float)t->v2.p2 / resolution[1] - 1.0f;
		v_buffer[i*9 + 5] = 2.0f * (float)t->v2.p3 / resolution[2] - 1.0f;
		v_buffer[i*9 + 6] = 2.0f * (float)t->v3.p1 / resolution[0] - 1.0f;
		v_buffer[i*9 + 7] = 2.0f * (float)t->v3.p2 / resolution[1] - 1.0f;
		v_buffer[i*9 + 8] = 2.0f * (float)t->v3.p3 / resolution[2] - 1.0f;
		
		c_buffer[i*9 + 0] = (float)t->color[0] / 256.0f;
		c_buffer[i*9 + 1] = (float)t->color[1] / 256.0f;
		c_buffer[i*9 + 2] = (float)t->color[2] / 256.0f;
		c_buffer[i*9 + 3] = (float)t->color[0] / 256.0f;
		c_buffer[i*9 + 4] = (float)t->color[1] / 256.0f;
		c_buffer[i*9 + 5] = (float)t->color[2] / 256.0f;
		c_buffer[i*9 + 6] = (float)t->color[0] / 256.0f;
		c_buffer[i*9 + 7] = (float)t->color[1] / 256.0f;
		c_buffer[i*9 + 8] = (float)t->color[2] / 256.0f;
	}
}

void OpenGL_Example::render(vector<Triangle>* triangles, int* resolution, int* s_red, int* s_green, int* s_blue, float* s_depth) {
	
	GLFWwindow* window;
	
	// Initialize GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return ;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	
	// Open a window and create its OpenGL context
	window = glfwCreateWindow( resolution[1], resolution[2], "Tutorial 04 - Colored Cube", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return ;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return ;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "Rendering/OpenGL_common/TransformVertexShader.vertexshader", "Rendering/OpenGL_common/ColorFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(180.0f, 4.0f / 4.0f, 0.1f, 100.0f);
	// Camera matrix
	glm::mat4 View       = glm::lookAt(
								glm::vec3(1.5,0,0), // Camera is at (4,3,-3), in World Space
								glm::vec3(0,0,0), // and looks at the origin
								glm::vec3(0,0,1)  // Head is up (set to 0,-1,0 to look upside-down)
						   );
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model      = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP        = Projection * View * Model; // Remember, matrix multiplication is the other way around

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	vector<GLfloat> g_vertex_buffer_data(3*3*triangles->size());
	
	// One color for each vertex. They were generated randomly.
	vector<GLfloat> g_color_buffer_data(3*3*triangles->size());
	
	readTriangles(triangles, &g_vertex_buffer_data.front(), &g_color_buffer_data.front(), resolution);
	
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, g_vertex_buffer_data.size(), &g_vertex_buffer_data.front(), GL_STATIC_DRAW);

	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, g_color_buffer_data.size(), &g_color_buffer_data.front(), GL_STATIC_DRAW);
	
	// ---------------------------------------------
	// Render to Texture - specific code begins here
	// ---------------------------------------------
	int windowWidth = resolution[1];
     int windowHeight = resolution[2];
     
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures(1, &renderedTexture);
	
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	// Give an empty image to OpenGL ( the last "0" means "empty" )
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, windowWidth, windowHeight, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Poor filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// The depth buffer
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, windowWidth, windowHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

	// Always check that our framebuffer is ok
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return ;

	
	// The fullscreen quad's FBO
	static const GLfloat g_quad_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
	};

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Create and compile our GLSL program from the shaders
	GLuint quad_programID = LoadShaders( "Rendering/OpenGL_common/TransformVertexShader.vertexshader", "Rendering/OpenGL_common/ColorFragmentShader.fragmentshader"  );
	GLuint texID = glGetUniformLocation(quad_programID, "renderedTexture");
	GLuint timeID = glGetUniformLocation(quad_programID, "time");
	
	
//	int count = 0;
//	do{
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0,0,windowWidth,windowHeight); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		
		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, triangles->size()*3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
							
		for (int x = 0; x < resolution[1]; x++) {
			for (int y = 0; y < resolution[2]; y++) {
				std::vector< unsigned char > pick_color(1*1*3);
				glReadPixels(y , x , 1 , 1 , GL_RGB , GL_UNSIGNED_BYTE , &pick_color[0]);
				std::vector< float > pick_depth(1*1*1);
				glReadPixels(y , x , 1 , 1 , GL_DEPTH_COMPONENT , GL_FLOAT , &pick_depth[0]);
				
				s_red[x*resolution[1] + y] = (int)pick_color[0];
				s_green[x*resolution[1] + y] = (int)pick_color[1];
				s_blue[x*resolution[1] + y] = (int)pick_color[2];
				s_depth[x*resolution[1] + y] = (float)pick_depth[0];
				pick_color.clear();
				pick_depth.clear();
			}
		}	
		
		// Swap buffers
//		glfwPollEvents();
//		glfwSwapBuffers(window);
//		
//		if (count == 0)
//			break;
//		count++;
//	} // Check if the ESC key was pressed or the window was closed
//	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
//		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &colorbuffer);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);
	
	glDeleteFramebuffers(1, &FramebufferName);
	glDeleteTextures(1, &renderedTexture);
	glDeleteRenderbuffers(1, &depthrenderbuffer);
	glDeleteBuffers(1, &quad_vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return ;
}

