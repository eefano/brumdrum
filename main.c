#include <GLES3/gl31.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>


int windoww = 256, windowh = 256, mousel = GL_FALSE;

double mousex = 0.0;
double mousey = 0.0;

int SAMPLERATE=44100;
int BUFSIZE=16384;
int SMPSIZE=4096;

GLuint tex1;
GLuint tex2;
GLuint texs;
GLuint fb;
GLuint pb;
GLuint drawlist;



GLuint createshader(char *source,GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader,source);
    glCompileShader(shader);
    return shader;
}

void varpars(GLuint program, float aflex, float adamp, float bflex, float bdamp)
{
    glUseProgram(program);
    glUniform1f(glGetUniformLocation(program, "aflex"), aflex);
    glUniform1f(glGetUniformLocation(program, "adamp"), adamp);
    glUniform1f(glGetUniformLocation(program, "bflex"), bflex);
    glUniform1f(glGetUniformLocation(program, "bdamp"), bdamp);
    glUseProgram(0);
}

GLuint createprogram (GLuint vsh,GLuint fsh,int w,int h)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vsh);
    glAttachShader(program, fsh);
    glLinkProgram(program);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    glUniform1f(glGetUniformLocation(program, "du"), (1.0 / w));
    glUniform1f(glGetUniformLocation(program, "dv"), (1.0 / h));
    glUseProgram(0);
    varpars(program, 0.51, 0.995, 0.5, 0.995);
    return program;
}

GLuint createtexture (int w, int h)
{
    GLuint texture;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // IMPORTANT: texture must be COMPLETE (mipmaps must be specified..
    // or the following two parameters can be used if there are none)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, 0);
    return texture;
}

GLuint createpb(void)
{
    GLuint pb;
    glGenBuffers(1, &pb);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pb);
    glBufferData(GL_PIXEL_PACK_BUFFER, BUFSIZE, NULL, GL_DYNAMIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return pb;
}

GLuint createfb(GLuint tex1, GLuint tex2, GLuint texs)
{
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex2, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texs, 0);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glDrawBuffers(GL_COLOR_ATTACHMENT0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffers(GL_COLOR_ATTACHMENT1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.25, 0.0, 0.0, 0.0);
    glDrawBuffers(GL_COLOR_ATTACHMENT2);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fb;
}

GLuint createlist(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glBegin(GL_QUADS);
    glColor3f(1.0, 1.0, 1.0);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(-1.0, 1.0);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, -1.0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(1.0, -1.0);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(1.0, 1.0);
    glEnd();
    glEndList();
    return list;
}


void prepare(GLFWwindow *window, int w, int h)
{
    glEnable(GL_TEXTURE_2D);
    tex1 = createtexture(w,h);
    tex2 = createtexture(w,h);
    texs = createtexture(w,h);
    reshade(window,w,h);
    fb = createfb(tex1,tex2,texs);
    pb = createpb();
    drawlist = createlist();
}

void close_callback(GLFWwindow *window)
{
    glfwSetWindowShouldClose(window,GL_TRUE);
}
void size_callback(GLFWwindow *window, int w, int h)
{
    windoww = w; windowh = h;
    glViewport(0,0,w,h);
}
void mouse_callback(GLFWwindow *window, int b, int a, int m)
{
    if (b==GLFW_MOUSE_BUTTON_LEFT && a==GLFW_PRESS) mousel=GL_TRUE;
    if (b==GLFW_MOUSE_BUTTON_LEFT && a==GLFW_RELEASE) mousel=GL_FALSE;
}
void cursor_callback(GLFWwindow *window, double x, double y)
{
    mousex = x; mousey = y;
}

int main(int argc, char **argv)
{
    if(argc>=3)
    {
        windoww = atoi(argv[1]);
        windowh = atoi(argv[2]);
    }

    glfwInit();
    GLFWwindow* window = glfwCreateWindow(windoww,windowh,"mio",NULL,NULL);

    glfwSetWindowCloseCallback(window,&close_callback);
    glfwSetWindowSizeCallback(window,&size_callback);
    glfwSetMouseButtonCallback(window,&mouse_callback);
    glfwSetCursorPosCallback(window,&cursor_callback);
    glfwSwapInterval(1);

    prepare(window,windoww,windowh);


    while(glfwWindowShouldClose(window)==GL_FALSE)
    {



        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);

    return 0;


}

