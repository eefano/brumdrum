#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <jack/jack.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>


pthread_mutex_t lock;

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

int windoww = 256, windowh = 256, drumw = 128, drumh = 128, mousel = GL_FALSE, flip=0;
float mousec[4]={1.0, 1.0, 1.0, 1.0};

GLFWwindow* window;

const GLenum attachments[3]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_FRONT};

double mousex = 0.0;
double mousey = 0.0;

GLuint tex1;
GLuint tex2;
GLuint texs;
GLuint fb;
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
                "gl_FragColor = vec4(r,r,r,1.0);"
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, 0);
    printf("glTexImage2D %d\r\n",glGetError());
    return texture;
}


GLuint createfb(GLuint tex1, GLuint tex2)
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
    printf("glCheckFramebufferStatus %d\r\n",glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glClearColor(0.0, 0.0, 0.9999, 1.0);
    glDrawBuffers(1,&attachments[0]);
    printf("glDrawBuffers %d\r\n",glGetError());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffers(1,&attachments[1]);
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

void reshade(int w, int h)
{
    GLuint v1 = createshader(vshader, GL_VERTEX_SHADER);
    GLuint r = createshader(rshader, GL_FRAGMENT_SHADER);
    GLuint v2 = createshader(vshader, GL_VERTEX_SHADER);
    GLuint f = createshader(fshader, GL_FRAGMENT_SHADER);

    rprogram = createprogram(v1,r,w,h);
    fprogram = createprogram(v2,f,w,h);
}

void prepare(int w, int h, int smpsize)
{
    glEnable(GL_TEXTURE_2D);
    tex1 = createtexture(w,h);
    tex2 = createtexture(w,h);
    texs = createtexture(smpsize,1);
    reshade(w,h);
    fb = createfb(tex1,tex2);
    drawlist = createlist();
}

void close_callback(GLFWwindow *window)
{
    glfwSetWindowShouldClose(window,GL_TRUE);
}
void size_callback(GLFWwindow *window, int w, int h)
{
    windoww = w; windowh = h;
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


void jack_shutdown (void *arg)
{
}
void jack_init (void *arg)
{
}

int process (jack_nframes_t nframes, void *arg)
{
    jack_default_audio_sample_t *out;
    out = jack_port_get_buffer (output_port, nframes);

    pthread_mutex_lock(&lock);
    glfwMakeContextCurrent(window);

    glViewport(0, 0, drumw, drumh);
    glBindFramebuffer(GL_FRAMEBUFFER,fb);
    glUseProgram(fprogram);

    for(int i=0;i<nframes;i++,flip^=1)
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

        glBindTexture(GL_TEXTURE_2D,texs);
        glCopyTexSubImage2D(GL_TEXTURE_2D,0,i,0,0,0,1,1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    // SOUND BLOCK
    glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,out);
    //glReadPixels(0, 0, drumw, nframes/drumw, GL_RED, GL_FLOAT, out);

    glfwMakeContextCurrent(NULL);
    pthread_mutex_unlock(&lock);

    return 0;
}




int main(int argc, char **argv)
{
    pthread_mutex_init(&lock, NULL);

    glfwInit();
    window = glfwCreateWindow(windoww,windowh,"mio",NULL,NULL);

    glfwMakeContextCurrent(window);
    glewInit();

    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);

    glfwSetWindowCloseCallback(window,&close_callback);
    glfwSetWindowSizeCallback(window,&size_callback);
    glfwSetMouseButtonCallback(window,&mouse_callback);
    glfwSetCursorPosCallback(window,&cursor_callback);
    glfwSwapInterval(2);

    jack_status_t status;
    client = jack_client_open ("brumdrum", JackNullOption, &status, NULL);

    prepare(drumw,drumh,jack_get_buffer_size(client));

    glfwMakeContextCurrent(NULL);

    jack_set_thread_init_callback(client, jack_init, 0);
    jack_set_process_callback (client, process, 0);
    jack_on_shutdown (client, jack_shutdown, 0);

    output_port = jack_port_register (client, "output",
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);

    jack_activate(client);

    const char **ports = jack_get_ports (client, NULL, NULL,
                            JackPortIsPhysical|JackPortIsInput);

    jack_connect (client, jack_port_name (output_port), ports[6]);
    jack_connect (client, jack_port_name (output_port), ports[7]);

    while(glfwWindowShouldClose(window)==GL_FALSE)
    {
        glfwPollEvents();

        pthread_mutex_lock(&lock);
        glfwMakeContextCurrent(window);

        if(mousel==GL_TRUE)
        {
            mousel=GL_FALSE;

            int x = (int)((mousex * drumw) / windoww);
            int y = drumh - (int)((mousey * drumh) / windowh) - 1;

            glBindTexture(GL_TEXTURE_2D, tex2);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RED, GL_FLOAT, mousec);
        }

        glViewport(0, 0, windoww, windowh);
        glUseProgram(rprogram);
        glBindTexture(GL_TEXTURE_2D, tex2);
        glCallList(drawlist);

        glFlush();
        glfwSwapBuffers(window);

        glfwMakeContextCurrent(NULL);
        pthread_mutex_unlock(&lock);

    }
    jack_client_close(client);

    glfwDestroyWindow(window);

    pthread_mutex_destroy(&lock);

    return 0;

}

