#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>


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


char vshader[]= "varying vec2 v_texCoord;"
                "void main() {"
                "v_texCoord = gl_MultiTexCoord0.xy;"
                "gl_Position=ftransform();"
                "}";

char rshader[]= "uniform sampler2D tex;"
                "varying vec2 v_texCoord;"
                "void main() {"
                "float r = texture2D(tex, v_texCoord).r + 0.5;"
                "gl_FragColor = vec4(r,r,r,r);"
                "}";

char fshader[]= "varying vec2 v_texCoord;"
                "uniform sampler2D tex;"
                "uniform float du;"
                "uniform float dv;"
                "void main() {"
                "vec4 C = texture2D( tex, v_texCoord );"
                "vec4 E = texture2D( tex, vec2(v_texCoord.x + du, v_texCoord.y) );"
                "vec4 N = texture2D( tex, vec2(v_texCoord.x, v_texCoord.y + dv) );"
                "vec4 W = texture2D( tex, vec2(v_texCoord.x - du, v_texCoord.y) );"
                "vec4 S = texture2D( tex, vec2(v_texCoord.x, v_texCoord.y - dv) );"

                "float X = ((N.r+W.r+S.r+E.r) * 0.5 - C.g) * C.b ;"

                "gl_FragColor = vec4(X,C.r,C.b,1.0);"
                "}";


GLuint createshader(const GLchar *source,GLenum type)
{
    GLuint shader = glCreateShader(type);
    printf("glCreateShader %d\r\n",glGetError());
    glShaderSource(shader,1,&source,NULL);
    printf("glShaderSource %d\r\n",glGetError());
    glCompileShader(shader);
    printf("glCompileShader %d\r\n",glGetError());
    return shader;
}

GLuint createprogram (GLuint vsh,GLuint fsh,int w,int h)
{
    GLuint program = glCreateProgram();
    printf("glCreateProgram %d\r\n",glGetError());
    glAttachShader(program, vsh);
    printf("glAttachShader %d\r\n",glGetError());
    glAttachShader(program, fsh);
    printf("glAttachShader %d\r\n",glGetError());
    glLinkProgram(program);
    printf("glLinkProgram %d\r\n",glGetError());
    glUseProgram(program);
    printf("glUseProgram %d\r\n",glGetError());
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    glUniform1f(glGetUniformLocation(program, "du"), (1.0 / w));
    glUniform1f(glGetUniformLocation(program, "dv"), (1.0 / h));
    glUseProgram(0);
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
    printf("glTexImage2D %d\r\n",glGetError());
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
    printf("glGenFramebuffers %d\r\n",glGetError());
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    printf("glBindFramebuffer %d\r\n",glGetError());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);
    printf("glFramebufferTexture2D %d\r\n",glGetError());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex2, 0);
    printf("glFramebufferTexture2D %d\r\n",glGetError());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texs, 0);
    printf("glFramebufferTexture2D %d\r\n",glGetError());
    printf("glCheckFramebufferStatus %d\r\n",glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glClearColor(0.0, 0.0, 0.9, 1.0);
    glDrawBuffers(1,&attachments[0]);
    printf("glDrawBuffers %d\r\n",glGetError());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffers(1,&attachments[1]);
    printf("glDrawBuffers %d\r\n",glGetError());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffers(1,&attachments[2]);
    printf("glDrawBuffers %d\r\n",glGetError());
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
    GLuint v1 = createshader(vshader, GL_VERTEX_SHADER);
    GLuint r = createshader(rshader, GL_FRAGMENT_SHADER);
    GLuint v2 = createshader(vshader, GL_VERTEX_SHADER);
    GLuint f = createshader(fshader, GL_FRAGMENT_SHADER);

    rprogram = createprogram(v1,r,w,h);
    fprogram = createprogram(v2,f,w,h);
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

    glfwMakeContextCurrent(window);
    glewInit();

    glfwSetWindowCloseCallback(window,&close_callback);
    glfwSetWindowSizeCallback(window,&size_callback);
    glfwSetMouseButtonCallback(window,&mouse_callback);
    glfwSetCursorPosCallback(window,&cursor_callback);
    glfwSwapInterval(1);

    prepare(window,w,h);


    int flip=0;

    while(glfwWindowShouldClose(window)==GL_FALSE)
    {

        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER,fb);
        glUseProgram(fprogram);

        for(int i=0;i<1;i++,flip^=1)
        {
            if(flip)
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
        */

        glPopAttrib();
        glBindFramebuffer(GL_FRAMEBUFFER,0);

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

