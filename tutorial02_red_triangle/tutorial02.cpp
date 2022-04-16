// Include standard headers


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>


extern "C" {
// Get declaration for f(int i, char c, float x)




}
extern "C" {
// Get declaration for f(int i, char c, float x)
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}



GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
using namespace glm;

#include <common/shader.hpp>

static AVFormatContext *ifmt_ctx;
typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;

    AVFrame *dec_frame;
} StreamContext;
static StreamContext *stream_ctx;

static void logging(const char *fmt, ...)
{
    va_list args;
    fprintf( stderr, "LOG: " );
    va_start( args, fmt );
    vfprintf( stderr, fmt, args );
    va_end( args );
    fprintf( stderr, "\n" );
}
static void ppm_save(unsigned char* buf, int wrap, int xsize, int ysize, char* filename)
{
    FILE* f;
    int i;

    f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

    for (i = 0; i < ysize; i++)
    {
        fwrite(buf + i * wrap, 1, xsize*3, f);
    }

    fclose(f);
}
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    logging("save file %s",filename );
    f = fopen(filename,"w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
    logging("decode frame");

    int response = avcodec_send_packet(pCodecContext, pPacket);

    if (response < 0) {
        char  e[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(e,  AV_ERROR_MAX_STRING_SIZE, response);
        logging("ERROR while sending a packet to the decoder: %s", e);
        return response;
    }
    while (response >= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/30));
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            char  e[AV_ERROR_MAX_STRING_SIZE];
            av_make_error_string(e,  AV_ERROR_MAX_STRING_SIZE, response);
            logging("AVERROR %s",e);
            break;
        } else if (response < 0) {
            logging("Error while receiving a frame from the decoder: ");
            return response;
        }

        if (response >= 0) {
            logging(
                    "Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d]",
                    pCodecContext->frame_number,
                    av_get_picture_type_char(pFrame->pict_type),
                    pFrame->pkt_size,
                    pFrame->format,
                    pFrame->pts,
                    pFrame->key_frame,
                    pFrame->coded_picture_number
            );

            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "%s-%d.bmp", "frame", pCodecContext->frame_number);
            // Check if the frame is a planar YUV 4:2:0, 12bpp
            // That is the format of the provided .mp4 file
            // RGB formats will definitely not give a gray image
            // Other YUV image may do so, but untested, so give a warning
            if (pFrame->format != AV_PIX_FMT_YUV420P)
            {
                logging("Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB");
            }
            // save a grayscale frame into a .pgm file
            save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
        }
    }


    return  0;

}

static int open_input_file(const char *filename)
{
    int ret;
    unsigned int i;

    ifmt_ctx = NULL;
    AVFormatContext *pAVFormatContext = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }
    av_log(NULL, AV_LOG_INFO, "avformat_find_stream_info\n");
     av_calloc(ifmt_ctx->nb_streams, sizeof(*stream_ctx));
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            continue;
        }
        else
            av_log(NULL, AV_LOG_INFO, "find decoder for stream #%u\n", i);
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }


    }




    return 0;
}

int work(){
    int ret;
    AVPacket *packet;
     AVCodec *dec;
    unsigned int videostream_index=-1;
    unsigned int i;
    const char *url = "/tmp/vcbr.mp4";

    ifmt_ctx = NULL;
    packet = av_packet_alloc();

    AVFormatContext *pAVFormatContext = NULL;
    if ( avformat_open_input(&pAVFormatContext, url, NULL, NULL)< 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

/* select the video stream */
    videostream_index = av_find_best_stream(pAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (videostream_index < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    logging("video stream index %d", videostream_index);


    if(videostream_index<0)
    {
        logging("ERROR: no video stram find\n");
        return  1;
    }
    av_log(NULL, AV_LOG_INFO, "before while\n");
    while (av_read_frame(pAVFormatContext, packet)) {
        logging("av_read_frame");
        if(packet->stream_index == videostream_index) {
            av_log(NULL, AV_LOG_INFO, "decode video stream\n");
        }

       /// av_log(NULL, AV_LOG_INFO, "av_read_frame\n");
    }

    return 0;
}

int main( void )
{

    return work();



    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (!pFormatContext) {
        printf("ERROR could not allocate memory for Format Context");
        return -1;
    }
    printf(" allocate memory for Format Context\n");

    const char *url = "file:/tmp/vcbr.mp4";

    if (avformat_open_input(&pFormatContext, url, NULL, NULL) != 0) {
        printf("ERROR could not open the file");
        return -1;
    }
    std::cout<< "format: "<< pFormatContext->iformat->name << "; duration" << pFormatContext->duration << "; bit_rate" << pFormatContext->bit_rate << "\n";

    logging("finding stream info from format");

    if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
        logging("ERROR could not get the stream info");
        return -1;
    }
    AVCodec *pCodec = NULL;
    // this component describes the properties of a codec used by the stream i
    // https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
    AVCodecParameters *pCodecParameters =  NULL;
    int video_stream_index = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        logging("AVStream->time_base before open coded %d/%d", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
        logging("AVStream->r_frame_rate before open coded %d/%d", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
        logging("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
        logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

        logging("finding the proper decoder (CODEC) %d ", pLocalCodecParameters->codec_id);

        AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec==NULL) {
            logging("ERROR unsupported codec! %d", i);
            // In this example if the codec is not found we just skip it
            continue;
        }

        // when the stream is a video we store its index, codec parameters and codec
        logging("pLocalCodecParameters->codec_type %d",  i);
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (video_stream_index == -1) {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
                logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
            }


        } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            logging("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
        }

        // print its name, id and bitrate
        logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);

    }
    std::cout<< "video_stream_index" << video_stream_index << "\n";
    logging("video_stream_index " );
    if (video_stream_index == -1) {
        logging("File %s does not contain a video stream!");
        return -1;
    }
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext)
    {
        logging("failed to allocated memory for AVCodecContext");
        return -1;
    }

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
    {
        logging("failed to copy codec params to codec context");
        return -1;
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        logging("failed to open codec through avcodec_open2");
        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVFrame *pFrame =  av_frame_alloc();
    if (!pFrame)
    {
        logging("failed to allocated memory for AVFrame");
        return -1;
    }
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        logging("failed to allocated memory for AVPacket");
        return -1;
    }

    int response = 0;
    int how_many_packets_to_process = 30;

    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        // if it's the video stream
        if (pPacket->stream_index == video_stream_index) {
            logging("AVPacket->pts %" PRId64, pPacket->pts);
            response = decode_packet(pPacket, pCodecContext, pFrame);
            //if (response < 0)
              //  break;
            // stop it, otherwise we'll be saving hundreds of frames
            if (--how_many_packets_to_process <= 0) break;
        }
        // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
        av_packet_unref(pPacket);
    }

    logging("releasing all the resources");

    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);



    // Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Tutorial 02 - Red triangle", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );




    float vertices[] = {
            // треугольник 1
            -1.0f, -1.0f, 0.0f, -1.0f, 0.0f,0.0f, -0.0f, 0.0f,0.0f,

            // треугольник 2
            -1.0f, -1.0f, -0.0f,0.0f, -1.0f, 0.0f,-0.0f, 0.0f,0.0f,

            // треугольник 3
            0.1f, 0.8f, 0.0f,0.1f, 0.2f, 0.0f,0.5f, 0.8f,0.0f,

            // треугольник 4
            0.1f, 0.2f, 0.0f,0.5f, 0.2f, 0.0f,0.5f, 0.8f,0.0f,
    };

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    static const GLfloat g_color_buffer_data[] = {
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            1.0f,  1.0f,  0.0f,
            1.0f,  1.0f,  0.0f,
            1.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f

    };

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	do{

		// Clear the screen
		glClear( GL_COLOR_BUFFER_BIT );

		// Use our shader
		glUseProgram(programID);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
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
		//glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle
       // glLineWidth(5);


        glDrawArrays(GL_TRIANGLES, 0, 3*2);
        glLineWidth(5);
        glDrawArrays(GL_LINES, 6, 2*2);
        //glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}


