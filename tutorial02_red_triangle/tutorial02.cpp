// Include standard headers


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

///
#include <GL/glut.h>
#include "Texture.h"


extern "C" {
// Get declaration for f(int i, char c, float x)
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}


GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>


static AVFormatContext *ifmt_ctx;
typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;

    AVFrame *dec_frame;
} StreamContext;
static StreamContext *stream_ctx;

struct CfinalFrameData {
    int width;
    int height;
    unsigned char  *data;
    long frameNumber;
    int linesize;
};
static CfinalFrameData finalFrameData;
std::mutex finalFrameData_lock;


static long nowTime() {
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );
    return ms.count();
}

static void logging(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "LOG: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

//Save RGB image as PPM file format
static void ppm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename) {
    FILE *f;
    int i;

    f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

    for (i = 0; i < ysize; i++) {
        fwrite(buf + i * wrap, 1, xsize * 3, f);
    }

    fclose(f);
}


int work() {

    AVFormatContext *ctx_format = nullptr;
    AVCodecContext *ctx_codec = nullptr;
    AVCodec *codec = nullptr;
    AVFrame *frame = av_frame_alloc();
    int stream_idx;
    const char *fin = "/tmp/vcbr.mp4";
    AVStream *vid_stream = nullptr;
    AVPacket *pkt = av_packet_alloc();
    int ret;
    int sts;
    struct SwsContext *sws_ctx = NULL;


    if (int ret = avformat_open_input(&ctx_format, fin, nullptr, nullptr) != 0) {
        std::cout << 1 << std::endl;
        return ret;
    }
    if (avformat_find_stream_info(ctx_format, nullptr) < 0) {
        std::cout << 2 << std::endl;
        return -1; // Couldn't find stream information
    }
    av_dump_format(ctx_format, 0, fin, false);
    for (int i = 0; i < ctx_format->nb_streams; i++)
        if (ctx_format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            stream_idx = i;
            vid_stream = ctx_format->streams[i];
            break;
        }
    if (vid_stream == nullptr) {
        std::cout << 4 << std::endl;
        return -1;
    }
    std::cout << " framerate: " << vid_stream->avg_frame_rate.num << " " << vid_stream->avg_frame_rate.den << std::endl;
    codec = avcodec_find_decoder(vid_stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    ctx_codec = avcodec_alloc_context3(codec);

    if (avcodec_parameters_to_context(ctx_codec, vid_stream->codecpar) < 0)
        std::cout << 512;
    if (avcodec_open2(ctx_codec, codec, nullptr) < 0) {
        std::cout << 5;
        return -1;
    }
    sws_ctx = sws_getContext(ctx_codec->width,
                             ctx_codec->height,
                             ctx_codec->pix_fmt,
                             ctx_codec->width,
                             ctx_codec->height,
                             AV_PIX_FMT_RGB24,
                             SWS_BICUBIC,
                             NULL,
                             NULL,
                             NULL);
    //Allocate frame for storing image converted to RGB.
    ////////////////////////////////////////////////////////////////////////////
    AVFrame *pRGBFrame = av_frame_alloc();

    pRGBFrame->format = AV_PIX_FMT_RGB24;
    pRGBFrame->width = ctx_codec->width;
    pRGBFrame->height = ctx_codec->height;

    sts = av_frame_get_buffer(pRGBFrame, 0);

    if (sts < 0) {
        std::cout << 4444 << std::endl;
        return -1;  //Error!
    }

    // задержка на частоту кадров
    int frameDur = (vid_stream->avg_frame_rate.den * 1000) / vid_stream->avg_frame_rate.num;
    long lastFrameTime = 0;

    finalFrameData_lock.lock();

    finalFrameData.width = 0;
    finalFrameData.height = 0;

    finalFrameData.frameNumber = -1;
    finalFrameData_lock.unlock();

    int ii = 0;
    while (av_read_frame(ctx_format, pkt) >= 0) {

        if (pkt->stream_index == stream_idx) {


            int ret = avcodec_send_packet(ctx_codec, pkt);
            if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                std::cout << "avcodec_send_packet: " << ret << " " << AVERROR(EAGAIN) << " " << AVERROR_EOF
                          << std::endl;
                break;
            }
            std::cout << "  av_read_frame " << ret << " " << ii << std::endl;;
            while (ret >= 0) {
                ret = avcodec_receive_frame(ctx_codec, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    //std::cout << "avcodec_receive_frame: " << ret << std::endl;
                    break;
                }
                ii++;

                int64_t pts = av_rescale(frame->pts, 1000000, AV_TIME_BASE);
                int64_t now = av_gettime_relative();//- frame->start;

                sts = sws_scale(sws_ctx,                //struct SwsContext* c,
                                frame->data,            //const uint8_t* const srcSlice[],
                                frame->linesize,        //const int srcStride[],
                                0,                      //int srcSliceY,
                                frame->height,          //int srcSliceH,
                                pRGBFrame->data,        //uint8_t* const dst[],
                                pRGBFrame->linesize);   //const int dstStride[]);
                if (sts != frame->height) {
                    std::cout << "sts != frame->height " << std::endl;
                    return -1;  //Error!
                }

                // задержка на частоту кадров
                long thisFrameTime = lastFrameTime + frameDur;
                if (thisFrameTime > nowTime()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(thisFrameTime - nowTime()));
                }
                lastFrameTime = nowTime();

                char buf[1024];
                snprintf(buf, sizeof(buf), "/var/www/video-broadcast.space/%s%03d.ppm", "", ctx_codec->frame_number);
                //ppm_save(pRGBFrame->data[0], pRGBFrame->linesize[0], pRGBFrame->width, pRGBFrame->height, buf);
                finalFrameData_lock.lock();

                finalFrameData.width = pRGBFrame->width;
                finalFrameData.height = pRGBFrame->height;
                finalFrameData.data = pRGBFrame->data[0];
                finalFrameData.linesize = pRGBFrame->linesize[0];
                finalFrameData.frameNumber = ctx_codec->frame_number;
                finalFrameData_lock.unlock();
                //std::cout << "sizeof"<< sizeof(*pRGBFrame->data[0]) <<std::endl;
                av_frame_unref(frame);

            }

        }

        av_packet_unref(pkt);
    }
    av_frame_free(&pRGBFrame);
    avcodec_free_context(&ctx_codec);
    avformat_close_input(&ctx_format);


    return 0;
}


void av() {
    // do smth
    work();
    return;

}

int mainbak(void) {
    std::thread thr(av);


    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1280, 720, "Tutorial 05 - Textured Cube", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(26.25f), 16.0f / 9.0f, 0.1f, 50.0f);
    // Camera matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0.255f, 0, 3), // Camera is at (4,3,3), in World Space
            glm::vec3(0.255f, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around



    // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
    // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
    float f = 0.7f;
    //0.71848f;
    static const GLfloat g_vertex_buffer_data[] = {
            -1.0f, f, 0.0f,
            -1.0f, -f, 0.0f,
            1.0f, -f, 0.0f,

            -1.0f, f, 0.0f,
            1.0f, -f, 0.0f,
            1.0f, f, 0.0f,


    };

    // Two UV coordinatesfor each vertex. They were created with Blender.
    static const GLfloat g_uv_buffer_data[] = {
            0.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,

    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

    do {



        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // Load the texture using any two methods
        GLuint Texture = loadBMP_custom("uvtemplate.bmp");
        //GLuint Texture = loadDDS("uvtemplate.DDS");

        finalFrameData_lock.lock();
        std::cout << "render frame: " << finalFrameData.frameNumber << " " << std::endl;

        finalFrameData_lock.unlock();
        // Get a handle for our "myTextureSampler" uniform
        GLuint TextureID;
        if(finalFrameData.width==0 || true ) {
            TextureID = glGetUniformLocation(programID, "myTextureSampler");
            glBindTexture(GL_TEXTURE_2D, Texture);
        }
        else
        {


            char buf[1024];
            snprintf(buf, sizeof(buf), "/var/www/video-broadcast.space/10.ppm");

            ppm_save(finalFrameData.data, finalFrameData.linesize, finalFrameData.width, finalFrameData.height, buf);

            ////////////
            FILE* inFile = fopen (buf,"rb");
            int tmpint;
            char str[100];

            unsigned char* pixels;
            int width;
            int height;
            int numChannels;


            GLuint texName;

            glGenTextures (1, &texName);


            if (inFile == NULL)
            {
                printf ("Can't open input file %s. Exiting.\n",buf);
                exit (1);
            }

            fscanf (inFile,"P%d\n", &tmpint);

            if (tmpint != 6)
            {
                printf ("Input file is not ppm. Exiting.\n");
                exit (1);
            }
            // skip comments embedded in header

            fgets (str,100,inFile);
            while (str[0]=='#')
                fgets(str,100,inFile);

            sscanf (str,"%d %d",&width, &height);
            fgets (str,100,inFile);
            sscanf (str,"%d",&tmpint);

            if (tmpint != 255)
                printf("Warning: maxvalue is not 255 in ppm file\n");

            numChannels = 3;
            pixels = (unsigned char*) malloc (numChannels * width * height * sizeof (unsigned char));

            if (pixels == NULL)
            {
                printf ("Can't allocate image of size %dx%d. Exiting\n", width, height);
                exit (1);
            }
            else
                printf("Reading image %s of size %dx%d\n", "", width, height);


            fread (pixels, sizeof (unsigned char), numChannels * width * height, inFile);

            fclose (inFile);
            ////////////



            glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

            glBindTexture (GL_TEXTURE_2D, 1);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width,
                         height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         pixels);

            glEnable (GL_TEXTURE_2D);
            glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            glBindTexture (GL_TEXTURE_2D, texName);

            ///////////

          //  TextureID = glGetUniformLocation(programID, "myTextureSampler");
           // glBindTexture(GL_TEXTURE_2D, Texture);

           /* glGenTextures(1, &TextureID);
            glBindTexture(GL_TEXTURE_2D, Texture);
            int dt[12]={255,255,255,255,255,255,255,255,255,255,255,255};
            glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, finalFrameData.width, finalFrameData.height, 0, GL_BGR, GL_UNSIGNED_BYTE, finalFrameData.data);
        */
            }
        // Bind our texture in Texture Unit 0

        glActiveTexture(GL_TEXTURE0);


        ///
        //if(finalFrameData.width>0)
        //    glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, finalFrameData.width, finalFrameData.height, GL_RGB, GL_UNSIGNED_BYTE, finalFrameData.data);


        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(TextureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void *) 0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void *) 0                          // array buffer offset
        );

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0 * 3, 2 * 3); // 12*3 indices starting at 0 -> 12 triangles

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDeleteTextures(1, &Texture);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));


    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO and shader
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);

    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}
///////////////
const float fMinX = -5.0, fMinY = -5.0, fNearZ = 1.0,
        fMaxX = 5.0 , fMaxY = 5.0 , fFarZ = 10.0;

Texture ImageOne ;

void Init ()
{
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glEnable (GL_DEPTH_TEST);

    glGenTextures (1, &ImageOne.texName);


    char buf[1024];
    snprintf(buf, sizeof(buf), "/var/www/video-broadcast.space/102.ppm");

    ImageOne.ReadPPMImage(buf);
    ImageOne.Prepare(1) ;
}

void Reshape (int width, int height)
{
    std::cout << "reshape"<< width << " " << height <<std::endl ;
    glViewport (0, 0, width, height);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();

    glOrtho (fMinX, fMaxX, fMinY, fMaxY, fNearZ, fFarZ);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
}

void Display ()
{
   // std::cout << "Display";
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable (GL_TEXTURE_2D);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture (GL_TEXTURE_2D, ImageOne.texName);

    glBegin(GL_QUADS);
    glTexCoord2f(0,1);
    glVertex3f(-7.5,5,-8);
    glTexCoord2f(0,0);
    glVertex3f(-5.5,-5,-8);
    glTexCoord2f(1,0);
    glVertex3f(5,-5,-8);
    glTexCoord2f(1,1);
    glVertex3f(5,5,-8);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers ();
    glFlush ();
}

int main (int argc, char **argv)
{
    // init GLUT and create window
    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(-1,-1);
    glutInitWindowSize(1280,720);
    glutCreateWindow ("OpenGL - Rotating Cubes");

    Init ();

    // register callbacks
    glutDisplayFunc (Display);
    glutReshapeFunc (Reshape);
    glutIdleFunc (Display);     // used in animation

    // enter GLUT event processing cycle
    glutMainLoop();
    return 0;
}

