
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Assimp includes

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.


/*----------------------------------------------------------------------------
                   MESH TO LOAD
  ----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here

#define GROUND_MESH "../Meshes/ground.obj"
#define GROUND_TEX "../Textures/Wood_Planks.jpg"
#define GROUND_TEX_W 2048
#define GROUND_TEX_H 2048

#define TABLE_MESH "../Meshes/table.obj"
#define TABLE_TEX "../Textures/wood_grain_texture.jpg"
#define TABLE_TEX_W 900
#define TABLE_TEX_H 508

#define MUG_MESH "../Meshes/mug_small.obj"
#define MESH_DIR "../Meshes"
/*----------------------------------------------------------------------------
  ----------------------------------------------------------------------------*/

std::vector<float> g_vp, g_vn, g_vt;
std::vector<float> mat;

int g_point_count = 0;


// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;
GLuint ground_tex;
GLuint table_tex;

unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

//Middle of screen
int midWidth = width / 2;
int midHeight = height / 2;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;

float camX = 5.0;
float camY = 5.0;
float camZ = 8.0;
float orientX, lastX;
float orientY, lastY;

GLuint texArr[2];

float  delta;



#pragma region MESH LOADING
/*----------------------------------------------------------------------------
                   MESH LOADING FUNCTION
  ----------------------------------------------------------------------------*/
	
bool load_mesh (const char* file_name) {
  const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate); // TRIANGLES!
  if (!scene) {
    fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
    return false;
  }
  printf ("  %i animations\n", scene->mNumAnimations);
  printf ("  %i cameras\n", scene->mNumCameras);
  printf ("  %i lights\n", scene->mNumLights);
  printf ("  %i materials\n", scene->mNumMaterials);
  printf ("  %i meshes\n", scene->mNumMeshes);
  printf ("  %i textures\n", scene->mNumTextures);
  
  for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
    const aiMesh* mesh = scene->mMeshes[m_i];
    printf ("    %i vertices in mesh\n", mesh->mNumVertices);
    g_point_count += mesh->mNumVertices;
    for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
      if (mesh->HasPositions ()) {
        const aiVector3D* vp = &(mesh->mVertices[v_i]);
        //printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
        g_vp.push_back (vp->x);
        g_vp.push_back (vp->y);
        g_vp.push_back (vp->z);
      }
      if (mesh->HasNormals ()) {
        const aiVector3D* vn = &(mesh->mNormals[v_i]);
        //printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
        g_vn.push_back (vn->x);
        g_vn.push_back (vn->y);
        g_vn.push_back (vn->z);
      }
      if (mesh->HasTextureCoords (0)) {
        const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
        //printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
        g_vt.push_back (vt->x);
        g_vt.push_back (vt->y);
      }
      if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
      }
    }
  }

  /*int iNumMaterials = scene->mNumMaterials;
  vector<int> materialRemap(iNumMaterials);
  for (int i = 0; i < iNumMaterials; i++)
  {
	  const aiMaterial* material = scene->mMaterials[i];
	  int a = 5;
	  int texIndex = 0;
	  aiString path;
	  if (material->GetTexture(aiTextureType_DIFFUSE, texIndex, &path) == AI_SUCCESS)
	  {
		  string sDir = MESH_DIR;
		  string sTextureName = path.data;
		  string fullPath = sDir + sTextureName;
	  }
  }*/

  
  aiReleaseImport (scene);
  return true;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {   
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	const char* pShaderSource = readShaderSource( pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

GLuint loadTexture(const char *filename)
{
	GLuint texture;
	unsigned char *data;
	
	int width, height;
	int n = 0;
	data = stbi_load(filename, &width, &height, &n, 0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_DECAL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	return texture;
}

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferMesh(const char* mesh_loc) {
/*----------------------------------------------------------------------------
                   LOAD MESH HERE AND COPY INTO BUFFERS
  ----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	load_mesh (mesh_loc);
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, g_point_count * 3 * sizeof (float), &g_vp[0], GL_STATIC_DRAW);
	unsigned	 int vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, g_point_count * 3 * sizeof (float), &g_vn[0], GL_STATIC_DRAW);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	unsigned int vt_vbo = 0;
	glGenBuffers (1, &vt_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glBufferData (GL_ARRAY_BUFFER, g_point_count * 2 * sizeof (float), &g_vt[0], GL_STATIC_DRAW);
	
	unsigned int vao = 0;
	glBindVertexArray (vao);

	glEnableVertexAttribArray (loc1);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (loc2);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray (loc3);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	
}


#pragma endregion VBO_FUNCTIONS


void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram (shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");
	

	// Root of the Hierarchy
	//mat4 view = identity_mat4 ();
	mat4 view = look_at({ camX, camY, camZ }, {orientX, 0, orientY }, {0, 1, 0});
	mat4 persp_proj = perspective(80.0, (float)width/(float)height, 0.1, 100.0);
	mat4 model = identity_mat4 ();
	model = rotate_y_deg (model, rotate_y); 
	view = translate (view, vec3 (0.0, 0.0, -5.0f));

	// update uniforms & draw
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, model.m);
	//glBindTexture(GL_TEXTURE_2D, texArr[0]);
	glDrawArrays (GL_TRIANGLES, 0, g_point_count);
    glutSwapBuffers();
}


void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// rotate the model slowly around the y axis
	//rotate_y+=0.2f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	
	texArr[0] = loadTexture(GROUND_TEX);
	texArr[1] = loadTexture(TABLE_TEX);

	// load mesh into a vertex buffer array
	generateObjectBufferMesh(GROUND_MESH);	
	generateObjectBufferMesh(TABLE_MESH);
	generateObjectBufferMesh(MUG_MESH);
}

void keypress(unsigned char key, int x, int y) 
{

	if(key=='w')
	{
		camZ -= delta * 10;
		//float xRad, yRad;
		//yRad = orientY / (180 * 3.141592654f);
		//xRad = orientX / (180 * 3.141592654f);
		//camX += float(sin(yRad));
		//camZ -= float(cos(yRad));
		//camY -= float(sin(xRad));
	}
	if (key == 's')
	{
		camZ += delta * 10;
		//float xRad, yRad;
		//yRad = orientY / (180 * 3.141592654f);
		//xRad = orientX / (180 * 3.141592654f);
		//camX -= float(sin(yRad));
		//camZ += float(cos(yRad));
		//camY += float(sin(xRad));
	}
	if (key == 'd')
	{ 
		camX += delta * 10;
		//float yRad;
		//yRad = orientY / (180 * 3.141592654f);
		//camX += float(sin(yRad));
		//camZ += float(cos(yRad));
	}
	if (key == 'a') 
	{
		camX -= delta * 10;
		//float yRad;
		//yRad = orientY / (180 * 3.141592654f);
		//camX -= float(sin(yRad));
		//camZ -= float(cos(yRad));
	}
	//if (key == '5')
	//{
	//	camZ = 0;
	//	camX = 0;
	//	glutWarpPointer(midWidth, midHeight);
	//}
}

void handleMouseMove(int x, int y)
{
	float mouseSensitivity = 20.0;

	int horiz = x - midWidth;
	int vert = y - midHeight;

	orientX = horiz / mouseSensitivity;
	orientY = vert / mouseSensitivity;

	//int diffX = x - lastX;
	//int diffY = y - lastY;
	//lastX = x;
	//lastY = y;
	//orientX += (float)diffX;
	//orientY += (float)diffY;

	//Limits
	//Looking up
	if (orientX < -90.0)
	{
		orientX = -90.0;
	}
	//Lookng down
	if (orientX > 90.0) 
	{
		orientX = 90.0;
	}
	//Side to side
	if (orientY < -180.0)
	{
		orientY += 360.0;
	}
	if (orientY > 180.0)
	{
		orientY -= 360.0;
	}

	//glutWarpPointer(midWidth, midHeight);
}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutPassiveMotionFunc(handleMouseMove);

	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}












