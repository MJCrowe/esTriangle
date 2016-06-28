/*
 * Book:      OpenGL(R) ES 2.0 Programming Guide
 * Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
 * ISBN-10:   0321502795
 * ISBN-13:   9780321502797
 * Publisher: Addison-Wesley Professional
 * URLs:      http: *safari.informit.com/9780321563835
 *            http://www.opengles-book.com
 */

/*!
 * \file ESUtil.h
 * \brief A utility library for OpenGL ES. This library provides a
 *        basic common framework for the example applications in the
 *        OpenGL ES 2.0 Programming Guide.
 */
#ifndef ESUTIL_H
#define ESUTIL_H

/*
 *  Includes
 */
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif
   
/*
 *  Macros
 */
#define ESUTIL_API
#define ESCALLBACK

/* esCreateWindow flag - RGB color buffer */
#define ES_WINDOW_RGB           0
/* esCreateWindow flag - ALPHA color buffer */
#define ES_WINDOW_ALPHA         1 
/* esCreateWindow flag - depth buffer */
#define ES_WINDOW_DEPTH         2 
/* esCreateWindow flag - stencil buffer */
#define ES_WINDOW_STENCIL       4
/* esCreateWindow flat - multi-sample buffer */
#define ES_WINDOW_MULTISAMPLE   8


/*
 * Types
 */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef struct
{
    GLfloat   m[4][4];
} ESMatrix;

typedef struct _escontext
{
    /* Put your user data here. */
    void *userData;
  
    /* Window width */
    GLint width;
    
    /* Window height */
    GLint height;

    /* Window handle */
    EGLNativeWindowType hWnd;

    /* RPi display handle required for exit function */
    DISPMANX_DISPLAY_HANDLE_T dispman_display;

    /* EGL display */
    EGLDisplay eglDisplay;
      
    /* EGL context */
    EGLContext eglContext;

    /* EGL surface */
    EGLSurface eglSurface;

    /* Callbacks */
    void (ESCALLBACK *drawFunc)(struct _escontext *);
    void (ESCALLBACK *keyFunc)(struct _escontext *, unsigned char, int, int);
    void (ESCALLBACK *updateFunc)(struct _escontext *, float deltaTime);
} ESContext;


/*
 *  Public Functions
 */

/*!
 * \brief Initialize ES framework context.  This must be called before
 *        calling any other functions.
 * \param esContext Application context
 */
void ESUTIL_API esInitContext (ESContext *esContext);


/*!
 * \brief Create a window with the specified parameters.
 * \param esContext Application context
 * \param title Name for title bar of window
 * \param width Width in pixels of window to create
 * \param height Height in pixels of window to create
 * \param flags Bitfield for the window creation flags 
 *        ES_WINDOW_RGB - specifies that the color buffer should have
 *        R,G,B channels
 *        ES_WINDOW_ALPHA - specifies that the color buffer should have alpha
 *        ES_WINDOW_DEPTH - specifies that a depth buffer should be created
 *        ES_WINDOW_STENCIL - specifies that a stencil buffer should be created
 *        ES_WINDOW_MULTISAMPLE - specifies that a multi-sample buffer
 *        should be created
 * \return GL_TRUE if window creation is succesful, GL_FALSE otherwise
 */
GLboolean ESUTIL_API esCreateWindow(ESContext *esContext, const char *title, 
                                                                        GLint width, GLint height, GLuint flags);

/*!
 * \brief RPi Exit function the OpenGL ES application.
 * \param esContext Application context
 */
void ESUTIL_API esExit(ESContext *esContext);

/*!
 * \brief Start the main loop for the OpenGL ES application.
 * \param esContext Application context
 */
void ESUTIL_API esMainLoop(ESContext *esContext);

/*!
 * \brief Register a draw callback function to be used to render each frame
 * \param esContext Application context
 * \param drawFunc Draw callback function that will be used to render the scene
 */
void ESUTIL_API esRegisterDrawFunc(ESContext *esContext, 
                                                                   void (ESCALLBACK *drawFunc)(ESContext*));

/*!
 * \brief Register an update callback function.
 * This function is used to update on each time step.
 * \param esContext Application context
 * \param updateFunc Update callback function that will be used to
 *                   render the scene
 */
void ESUTIL_API esRegisterUpdateFunc(ESContext *esContext, 
                                                                         void (ESCALLBACK *updateFunc)(ESContext*, float));

/*!
 * \brief Register an keyboard input processing callback function.
 * \param esContext Application context
 * \param keyFunc Key callback function for application processing of
 *                keyboard input
 */
void ESUTIL_API esRegisterKeyFunc(ESContext *esContext, 
                                                                  void (ESCALLBACK *drawFunc)(ESContext*, unsigned char, int, int));

/*!
 * \brief Log a message to the debug output for the platform
 * \param formatStr Format string for error log.  
 */
void ESUTIL_API esLogMessage (const char *formatStr, ...);


/*!
 * \brief Load a shader.
 * This function will load the shader source, compile it, check for
 * compile errors, and print error messages to output log.
 * \param type Type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 * \param shaderSrc Shader source string
 * \return A new shader object on success, 0 on failure
 */
GLuint ESUTIL_API esLoadShader(GLenum type, const char *shaderSrc);


/*!
 * \brief Create and link a program.
 * This function loads a vertex and fragment shader, creates a program
 * object, and links the program. Any errors are output to log.
 * \param vertShaderSrc Vertex shader source code
 * \param fragShaderSrc Fragment shader source code
 * \return A new program object linked with the vertex/fragment shader
 *         pair, 0 on failure
 */
GLuint ESUTIL_API esLoadProgram(const char *vertShaderSrc, 
                                                                const char *fragShaderSrc);


/*!
 * \brief Generates geometry for a sphere.  
 * This function allocates memory for the vertex data and stores the
 * results in the arrays.  It also Generates an index list for a
 * TRIANGLE_STRIP
 * \param numSlices The number of slices in the sphere
 * \param vertices If not NULL, will contain array of float3 positions
 * \param normals If not NULL, will contain array of float3 normals
 * \param texCoords If not NULL, will contain array of float2 texCoords
 * \param indices If not NULL, will contain the array of indices for
 *                the triangle strip
 * \param nvertices Pointer to the number of vertices.
 * \return The number of indices required for rendering the buffers
 *         (the number of indices stored in the indices array if it is
 *         not NULL) as a GL_TRIANGLE_STRIP
 */
int ESUTIL_API esGenSphere(int numSlices, float radius, 
   GLfloat **vertices, GLfloat **normals, 
   GLfloat **texCoords, GLushort **indices, GLuint *nvertices );

/*!
 * \brief Generates geometry for a cube.  
 * This function will allocate memory for the vertex data and stor the
 * results in the arrays.  It will also generate index list for a
 * TRIANGLE_STRIP.
 * \param scale The size of the cube, use 1.0 for a unit cube.
 * \param vertices If not NULL, will contain array of float3 positions
 * \param normals If not NULL, will contain array of float3 normals
 * \param texCoords If not NULL, will contain array of float2 texCoords
 * \param indices If not NULL, will contain the array of indices for
 *                the triangle strip
 * \param nvertices Pointer to the number of vertices.
 * \return The number of indices required for rendering the buffers
 *         (the number of indices stored in the indices array if it is
 *         not NULL) as a GL_TRIANGLES
 */
int ESUTIL_API esGenCube ( float scale, GLfloat **vertices, GLfloat **normals,
                           GLfloat **texCoords, GLushort **indices, GLuint *nvertices ) ;

/*!
 * \brief Loads a 24-bit TGA image from a file.
 * \param fileName Name of the file on disk
 * \param width Width of loaded image in pixels
 * \param height Height of loaded image in pixels
 * \return Pointer to loaded image.  NULL on failure. 
 */
char *ESUTIL_API esLoadTGA(char *fileName, int *width, int *height);


/*!
 * \brief Multiplies and scales a matrix.
 * This function multiplies the matrix specified by result with a
 * scaling matrix and return new matrix in result.
 * \param result Specifies the input matrix.  Scaled matrix is
 *               returned in result.
 * \param sx Scaling factor along x-axis.
 * \param sy Scaling factor along the y-axis.
 * \param sz Scaling factor along the z-axis.
 */
void ESUTIL_API esScale(ESMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz);

/*!
 * \brief Multiplies a matrix by a translation matrix.
 * The new matirx is returned in \c result.
 * \param result Specifies the input matrix. Translated matrix is
 *               returned in result.
 * \param tx Translation along the x-axis.
 * \param ty Translation along the y-axis.
 * \param tz Translation along the z-axis.
 */
void ESUTIL_API esTranslate(ESMatrix *result, 
                                                        GLfloat tx, GLfloat ty, GLfloat tz);

/*!
 * \brief Multiplies a matrix with a rotation matrix.
 * The result is returned in result.
 * \param result Specifies the input matrix. The rotated matrix is
 *               returned in result.
 * \param angle Specifies the angle of rotation, in degrees.
 * \param x The x-coordinate of the rotation axis.
 * \param y The y-coordinate of the rotation axis.
 * \param z The z-coordinate of the rotation axis.
 */
void ESUTIL_API esRotate(ESMatrix *result, GLfloat angle, 
                         GLfloat x, GLfloat y, GLfloat z);

/*!
 * \brief Multiplies a matrix by a perspective matrix.
 * The resulting matrix is returned in result
 * \param result Specifies the input matrix. The new matrix is
 *               returned in result.
 * \param left Coordinate for the left vertical clipping plane.
 * \param right Coordinate for the right vertical clipping plane.
 * \param bottom Coordinate for the bottom horizontal clipping plane.
 * \param top Coordinate for top horizontal clipping plane.
 * \param nearZ Distance to the near depth clipping plane. This
 *              distance must be positive.
 * \param farZ Distance to the far depth clipping plane. This distance
 *             must be positive.
 */
void ESUTIL_API esFrustum(ESMatrix *result, float left, float right, 
                                                  float bottom, float top, float nearZ, float farZ);

/*!
 * \brief Multiplies a matrix with a perspective matrix.
 * The result is returned in result
 * \param result Specifies the input matrix. The new matrix is
 *               returned in result.
 * \param fovy Field of view y angle in degrees
 * \param aspect Aspect ratio of screen
 * \param nearZ Near plane distance
 * \param farZ Far plane distance
 */
void ESUTIL_API esPerspective(ESMatrix *result, float fovy, float aspect, 
                                                          float nearZ, float farZ);

/*!
 * \brief Multiplies a  matrix with an orthograpic perspective matrix.
 * The result is returned in result
 * \param result Specifies the input matrix.  new matrix is returned in result.
 * \param left Coordinate for the left vertical clipping plane.
 * \param right Coordinate for the right vertical clipping plane.
 * \param bottom Coordinate for the bottom horizontal clipping plane.
 * \param top Coordinate for top horizontal clipping plane.
 * \param nearZ Distance to the near depth clipping plane. This
 *              distance must be positive.
 * \param farZ Distance to the far depth clipping plane. This distance
 *             must be positive.
 */
void ESUTIL_API esOrtho(ESMatrix *result, float left, float right, 
                                                float bottom, float top, float nearZ, float farZ);

/*!
 * \brief Multiplies two matrices.
 The result is stored in \c result.
 * \param result Returns multiplied matrix.
 * \param srcA First input matrix.
 * \param srcB Second input matrix.
 */
void ESUTIL_API esMatrixMultiply(ESMatrix *result, 
                                 ESMatrix *srcA, ESMatrix *srcB);

/*!
 * \brief Returns an indentity matrix.
 * The new matrix is stored in \c result.
 * \param result Returns identity matrix.
 */
void ESUTIL_API esMatrixLoadIdentity(ESMatrix *result);

#ifdef __cplusplus
}
#endif

#endif // ESUTIL_H
