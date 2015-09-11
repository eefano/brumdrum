#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>


int windoww = 256, windowh = 256, mousel = GL_FALSE;
float mousec[4]={1.0, 1.0, 1.0, 1.0};

const GLenum attachments[3]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};

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

GLuint rprogram;
GLuint fprogram;
GLuint sprogram;


char vshader[]="";
char rshader[]="";
char fshader[]="";
char sshader[]="";


GLuint createshader(const GLchar *source,GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader,1,&source,NULL);
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
    glDrawBuffers(1,&attachments[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffers(1,&attachments[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.25, 0.0, 0.0, 0.0);
    glDrawBuffers(1,&attachments[2]);
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

void blit(GLuint texture, GLuint list)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glCallList(list);
}

void reshade(int w, int h)
{
    rprogram = createprogram(createshader(vshader, GL_VERTEX_SHADER),
                             createshader(rshader, GL_FRAGMENT_SHADER), w, h);
    fprogram = createprogram(createshader(vshader, GL_VERTEX_SHADER),
                             createshader(fshader, GL_FRAGMENT_SHADER), w, h);
    sprogram = createprogram(createshader(vshader, GL_VERTEX_SHADER),
                             createshader(sshader, GL_FRAGMENT_SHADER), w, h);
}

void prepare(GLFWwindow *window, int w, int h)
{
    glEnable(GL_TEXTURE_2D);
    tex1 = createtexture(w,h);
    tex2 = createtexture(w,h);
    texs = createtexture(w,h);
    reshade(w,h);
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

    int w = windoww;
    int h = windowh;

    glfwInit();
    GLFWwindow* window = glfwCreateWindow(windoww,windowh,"mio",NULL,NULL);

    glfwSetWindowCloseCallback(window,&close_callback);
    glfwSetWindowSizeCallback(window,&size_callback);
    glfwSetMouseButtonCallback(window,&mouse_callback);
    glfwSetCursorPosCallback(window,&cursor_callback);
    glfwSwapInterval(1);

    prepare(window,w,h);


    while(glfwWindowShouldClose(window)==GL_FALSE)
    {
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER,fb);
        glUseProgram(fprogram);

        for(int i=0;i<SMPSIZE;i++)
        {
            if(i&1)
            {
                glDrawBuffers(1,&attachments[1]);
                glBindTexture(GL_TEXTURE_2D,tex1);
            }
            else
            {
                glDrawBuffers(1,&attachments[0]);
                glBindTexture(GL_TEXTURE_2D,tex2);
            }

            glCallList(drawlist);

            //glDrawBuffers(1,&attachments[2]);
            //glCallList(minilist[i]);
        }

        // SOUND BLOCK

        /*
        glBindBuffer(GL_PIXEL_PACK_BUFFER,pb);
        glReadBuffer(GL_COLOR_ATTACHMENT2);
        glReadPixels(0, 0, 256, 16, GL_RED, GL_FLOAT, 0);

        //  mappy = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        // (AL10/alBufferData b EXTFloat32/AL_FORMAT_MONO_FLOAT32 mappy SAMPLERATE))

        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
        glReadBuffer(GL_FRONT);

        glPopAttrib();
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        */

        // RENDER BLOCK

        if(mousel)
        {
            int x = (int)((mousex * w) / windoww);
            int y = h - (int)((mousey * h) / windowh) - 1;

            glBindTexture(GL_TEXTURE_2D, tex1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_FLOAT, mousec);
        }

        glUseProgram(rprogram);
        blit(tex1,drawlist);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);

    return 0;


}

