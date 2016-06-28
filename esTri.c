/* 
   Original code from OpenGL® ES 2.0 Programming Guide 'Hello Triangle' example.
   Uses the all ESUTIL functions to test & simplify this program's source code.
   Hence the RPi vc_dispmanx functions are removed from here.
   And uses my utility functions module for timer/keyboard/random fns.

Extracted from glGetString() on Pi0:
GL Vendor    :'Broadcom'.
GL Renderer  :'VideoCore IV HW'.
GL Version   :'OpenGL ES 2.0'.
GLSL Version :'OpenGL ES GLSL ES 1.00'.
GL Extensions:'GL_OES_compressed_ETC1_RGB8_texture GL_OES_compressed_paletted_texture
 GL_OES_texture_npot GL_OES_depth24 GL_OES_vertex_half_float GL_OES_EGL_image 
 GL_OES_EGL_image_external GL_EXT_discard_framebuffer GL_OES_rgb8_rgba8 GL_OES_depth32
 GL_OES_mapbuffer GL_EXT_texture_format_BGRA8888 GL_APPLE_rgb_422 GL_EXT_debug_marker'.

gl_FragColor deprecated after version 120. (=GLSL v1.20).

  Modified by M.J.Crowe 2016 UK.

  22/6/16 v1   Original flickers badly at 30fps because of two eglSwapBuffers()!!.
  24/6/16 v1.1 Corrected, added routines options, myMainLoop.
  25/6/16 v1.2 Correctly rotating textured cube.  
  25/6/16 v1.3 Correctly rotating vertex-coloured cube using VBOs. 
  26/6/16 v1.4 Added routine rotating textured cube not using VBOs.
  27/6/16 v1.5 Added routine rotating vertex-coloured sphere using VBOs.
*/


#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <limits.h>

#include "ESUtil.h"
#include "utils.h"

#define VERSION  "esTri v1.5: "

// Routines available :
// 1 = Original red triangle.
// 2 = Rotating vertex-coloured ES cube.
// 3 = Rotating textured ES cube.
// 4 = Rotating vertex-coloured ES Sphere
#define DEF_ROUTINE         1         // Which routine to display.

#define DEF_PERIOD          5.0f      // Default display period in seconds.


#define MICRO         1000000.0       // Microseconds in a second. 

// Maximum number of object types that can be setup.
#define MAXNOBJECTS        10

#define MAXNVBOIDS         10

#define BUF_OFFSET(i)   ((void *)(i))


typedef struct {
    GLuint   nv ;              // no. of vertices
    GLuint   ni ;              // no. of indices
    GLfloat  *v ;              // vertices
    GLfloat  *n ;              // normals
    GLfloat  *t ;              // texture coordinates
    GLfloat  *c ;              // colour(r,g,b) per vertex
    GLushort *i ;              // indices
    GLuint   program ;         // Vertex/Fragmenter Shader program handle.
    GLuint   vboIds[MAXNVBOIDS] ;  // five possible VBO ids(V/N/C/TC/I)
    GLuint   nvboIds ;         // no. of vboIds setup.
    ESMatrix modelMat ;        // model matrix
    ESMatrix viewMat ;         // view matrix
    ESMatrix projMat ;         // projection matrix
    ESMatrix mvpMat ;          // model*view*projection matrix
    GLuint   mvpId ;           // MVP matrix id handle
} OBJECT_T ;


// Note : This is an user-defined sub-structure of the main ESContext structure
// from ESUtil.h.
typedef struct {

    // Input parameters
    float    period;                // Display time in seconds.
    int      routine;               // Which routine to run?

    OBJECT_T object[MAXNOBJECTS] ;  // only using one for now.
    int      obj ;                  // current object index number
    int      nobjs ;                // number of objects setup

    // Handle to a program object  
    GLuint   programObject;         // Vertex/Fragmenter Shader program handle.

    int      keyboard_fd;           // Keyboard file descriptor.          
    int      count;                 // Loop count
    double   etime;                 // Elapsed time (us)
    int      toexit;                // Set to exit

    float    aspect;                // screen aspect ratio

    char    *image;
    int      width;                 // image size
    int      height;
    GLuint   textureId ;            // Texture handle

// Probably should be in OBJECT_T.   
    GLint    samplerLoc;            // Textured sampler location
    GLint    positionLoc;           // Attribute locations
    GLint    colourLoc; 
    GLint    texCoordLoc;

} UserData;


// Contains all display/user state data. Both required in the exit function.
ESContext esContext;
ESContext *esContextp = &esContext ;
UserData  userData;



//------------------------------------------------------------------------------


/***********************************************************
 * Name: parse
 *
 * Arguments:
 *     argc - no. of input parameters.
 *     argv - pointer to input strings.
 *     ESContext *esContext - holds display/user data.
 *
 * Description: Function to extract input parameters..
 *
 * Returns: void
 *
 ***********************************************************/
static void parse(int argc, char **argv, ESContext *esContext)
{
    UserData *user = esContext->userData;

    // Set up the default values.
    user->routine = DEF_ROUTINE ;
    user->period = DEF_PERIOD ;

    if ( argc > 1 ) {
        if ( *argv[1] == '?' ) {
            printf("Usage : %s <Routine> <Period(s)>\n",argv[0]) ;
            printf("Routines available :\n") ;
            printf("  1 = Original red triangle.\n") ;
            printf("  2 = Coloured rotating cube.\n") ;
            printf("  3 = Textured rotating cube.\n") ;
            printf("  4 = Coloured rotating sphere.\n") ;
            exit(0) ;
        }
        if ( isdigit(*argv[1]) )    // Limited to single digit!
            user->routine = (uint32_t) atoi(argv[1]) ;   
        if ( argc > 2 ) {
            GLfloat fP = (GLfloat) atof(argv[2]) ;
            if ( fP > 0.001f ) user->period = fP ;   
        }	
    }

    printf("Routine : %u\nPeriod : %.3fs\n",user->routine,user->period) ;
//   exit(0) ;   

} // parse()





/***********************************************************
 * Name: exit_func
 *
 * Arguments: None.
 *
 * Description: Functions to run on exit.
 *              Function to be passed to atexit().
 * Returns: void
 *
 ***********************************************************/

static void exit_func(void)
{
    UserData *user = esContextp->userData;
    OBJECT_T *ob = NULL ;
    int i ;

//    printf("Exiting...\n") ;
//    printf("Deleting %d objects...\n",user->nobjs) ;
   
    for ( i = 0 ; i < user->nobjs ; ++i ) {
        ob = &user->object[i] ;
        if ( ob ) {
            if ( ob->v ) free( ob->v ) ;
            if ( ob->n ) free( ob->n ) ;
            if ( ob->t ) free( ob->t ) ;
            if ( ob->i ) free( ob->i ) ;
            if ( ob->c ) free( ob->c ) ;
            if ( ob->program != user->programObject )
                glDeleteProgram( ob->program ) ;
            if ( ob->nvboIds > 0 )
                glDeleteBuffers(ob->nvboIds, ob->vboIds) ;
        }
    }
//    printf("Deleted %d objects.\n",user->nobjs) ;

    glDeleteProgram( user->programObject ) ;
//    printf("Deleted program object.\n") ;

    // Close RPi display.
    esExit( esContextp ) ;
//    printf("Closed display.\n") ;

    // Restore terminal settings.
    restore_terminal() ;

} // exit_func()


/*
static void printVertices(OBJECT_T *ob, int obj)
{
    int i ;
    GLfloat  *vp = ob->v ;
    GLushort *ip = ob->i ;

    printf("Object %d:\n",obj) ;
    for ( i = 0 ; i < ob->nv ; ++i, vp += 3 ) {
        printf("v%d : (%.3f,%.3f,%.3f)\n",i,vp[0],vp[1],vp[2]) ;
    } // each vertex
    for ( i = 0 ; i < ob->ni ; ++i, ip += 1 ) {
        printf("%d:%u,",i,ip[0]) ;
    } // each index
    printf("\n") ;
    
} //  printVertices
*/



///
// Create a simple width x height texture image.
//
GLuint loadTexture2D(char *image, int width, int height )
{
   // Texture object handle
   GLuint textureId;

   // Use tightly packed data
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

   // Generate a texture object
   glGenTextures ( 1, &textureId );

   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );

   // Load the texture
   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 
		  0, GL_RGB, GL_UNSIGNED_BYTE, image );

   // Set the filtering mode
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return textureId;

} // loadTexture2D







// Set up Vertex Buffer Objects(vertices/normals/textureCoordinates/colours).
// Working with VBOs which uses vertex data preloaded into GPU memory.
// Currently sets up V/C/I VBO buffers for draw_coloured_cube().
static void init_withVBOs(UserData *user, OBJECT_T *ob) 
{
    GLsizeiptr nvbytes = ob->nv * sizeof( GLfloat ) * 3 ;
//    GLsizeiptr ntbytes = ob->nv * sizeof( GLfloat ) * 2 ;
    GLsizeiptr nibytes = ob->ni * sizeof( GLushort ) ;
//    GLfloat color[3] = { 1.0f, 1.0f, 1.0f } ;


    ob->nvboIds = 5 ; 
    glGenBuffers(ob->nvboIds, ob->vboIds) ;

    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[0]) ;
    glBufferData(GL_ARRAY_BUFFER, nvbytes, ob->v, GL_STATIC_DRAW) ;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob->vboIds[4]) ;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nibytes, ob->i, GL_STATIC_DRAW) ;

//    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[1]) ;
//    glBufferData(GL_ARRAY_BUFFER, nvbytes, ob->n, GL_STATIC_DRAW) ;

    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[2]) ;
    glBufferData(GL_ARRAY_BUFFER, nvbytes, ob->c, GL_STATIC_DRAW) ;

//    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[3]) ;
//    glBufferData(GL_ARRAY_BUFFER, ntbytes, ob->t, GL_STATIC_DRAW) ;

    // Load the vertex position
    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[0]) ;
    glEnableVertexAttribArray(user->positionLoc) ;
    glVertexAttribPointer(user->positionLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
 
    // Load the vertex color
    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[2]) ;
    glEnableVertexAttribArray(user->colourLoc) ;
    glVertexAttribPointer(user->colourLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

/* 
    // Load the texture coordinate
    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[3]) ;
    glEnableVertexAttribArray(user->texCoordLoc) ;
    glVertexAttribPointer(user->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0 );
   
    // Bind the texture
    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture ( GL_TEXTURE_2D, user->textureId );

    // Set the sampler texture unit to 0
    glUniform1i( user->samplerLoc, 0 );
*/

//    glVertexAttrib3fv(user->colourLoc, color) ;
//    glUniform3f(user->colourLoc, 1.0f, 0.0f, 0.0f ) ;

    ob->mvpId = glGetUniformLocation(ob->program, "MVP") ;

} // init_withVBOs




// Set up without VBOs but slower. Working without VBOs.
// This loads vertex data into GPU memory every glDrawElements() call.
// Currently sets up V/C/I generic buffers for draw_textured_cube().
static void init_withoutVBOs(UserData *user, OBJECT_T *ob) 
{

    glBindBuffer(GL_ARRAY_BUFFER, 0) ;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) ;

    // Load the vertex position
    glVertexAttribPointer( user->positionLoc, 3, GL_FLOAT, GL_FALSE, 0, ob->v );
    // Load the texture coordinate
    glVertexAttribPointer( user->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, ob->t );

    glEnableVertexAttribArray( user->positionLoc );
    glEnableVertexAttribArray( user->texCoordLoc );

    // Bind the texture
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, user->textureId );

    // Set the sampler texture unit to 0
    glUniform1i( user->samplerLoc, 0 );

    ob->mvpId = glGetUniformLocation(ob->program, "MVP") ;

} // init_withoutVBOs






static int initialise_coloured_cube(ESContext *esContext)
{
    UserData *user = esContext->userData;
    int obj = user->nobjs ;                 // A new object
    OBJECT_T *ob = NULL ;
    int i , ret = 0 ;
    GLfloat *cp = NULL ;

    if ( obj >= MAXNOBJECTS ) {
        printf("Not initialise: Reached maximum no. of objects %d!\n",obj) ; 
        return 0 ;
    }
    ob = &user->object[obj] ;

    ob->ni = esGenCube(2.0,&ob->v,&ob->n,&ob->t,&ob->i,&ob->nv) ;

//    printVertices(ob,obj) ;

    // Setup colour vertices.
    ob->c = calloc( 3 * ob->nv, sizeof(GLfloat) );
    cp = ob->c ;
    for ( i = 0 ; i < ob->nv ; ++i, cp += 3 ) {
//         cp[i % 3] = 1.0f ;   // Red/Green/Blue vertices cyclically
         cp[0] = urandom(255) / 255.0f ;   
         cp[1] = urandom(255) / 255.0f ;   
         cp[2] = urandom(255) / 255.0f ;   
    } // each vertex

    ob->program = user->programObject ;  // for now use main shaders

    init_withVBOs(user,ob) ;

    user->obj = obj ;   // current object index number
    user->nobjs++ ;

    return ret ;   

} // initialise_coloured_cube



static int initialise_textured_cube(ESContext *esContext)
{
    UserData *user = esContext->userData;
    int obj = user->nobjs ;                 // A new object
    OBJECT_T *ob = NULL ;
    int ret = 0 ;

    if ( obj >= MAXNOBJECTS ) {
        printf("Not initialise: Reached maximum no. of objects %d!\n",obj) ; 
        return 0 ;
    }
    ob = &user->object[obj] ;

    ob->ni = esGenCube(2.0,&ob->v,&ob->n,&ob->t,&ob->i,&ob->nv) ;

//    printVertices(ob,obj) ;

    ob->program = user->programObject ;  // for now use main shaders

    init_withoutVBOs(user,ob) ;

    user->obj = obj ;   // current object index number
    user->nobjs++ ;

    return ret ;   

} // initialise_textured_cube








static int initialise_coloured_sphere(ESContext *esContext)
{
    UserData *user = esContext->userData;
    int obj = user->nobjs ;                 // A new object
    OBJECT_T *ob = NULL ;
    int i , ret = 0 ;
    GLfloat *cp = NULL ;

    if ( obj >= MAXNOBJECTS ) {
        printf("Not initialise: Reached maximum no. of objects %d!\n",obj) ; 
        return 0 ;
    }
    ob = &user->object[obj] ;

    // Use <=350 slices = 61776 vertices & 367500 indices!
    ob->ni = esGenSphere(350,1.0,&ob->v,&ob->n,&ob->t,&ob->i,&ob->nv) ;

    printf("Created sphere: %d vertices and %d indices.\n",ob->nv,ob->ni) ;
    if ( ob->nv > USHRT_MAX ) {
        fprintf(stderr,"Generated too many vertices, GPU limit is %d for the USHORT indices!!\n",USHRT_MAX) ;
        exit(0) ;
    }

//    printVertices(ob,obj) ;

    // Setup colour vertices.
    ob->c = calloc( 3 * ob->nv, sizeof(GLfloat) );
    cp = ob->c ;
    for ( i = 0 ; i < ob->nv ; ++i, cp += 3 ) {
//         cp[i % 3] = 1.0f ;   // Red/Green/Blue vertices cyclically
         cp[0] = urandom(255) / 255.0f ;   
         cp[1] = urandom(255) / 255.0f ;   
         cp[2] = urandom(255) / 255.0f ;   
    } // each vertex

    ob->program = user->programObject ;  // for now use main shaders

    init_withVBOs(user,ob) ;

    user->obj = obj ;   // current object index number
    user->nobjs++ ;

    return ret ;   

} // initialise_coloured_sphere








static int initialise_objects(ESContext *esContext)
{
    UserData *user = esContext->userData;
    int ret = 0 ;


    switch ( user->routine ) {
        case 1 :
            break ;
        case 2 :  
            ret = initialise_coloured_cube(esContext) ;
            break ;
        case 3 :  
            ret = initialise_textured_cube(esContext) ;
            break ;
        case 4 :  
            ret = initialise_coloured_sphere(esContext) ;
            break ;
        default :
            break ;
    }

    return ret ;   

} // initialise_objects




static void load_image(UserData *uData)
{
    static char *imagefn = "goldfish.tga" ;

    uData->image = esLoadTGA(imagefn, &uData->width, &uData->height);

    if (uData->image == NULL) {
	fprintf(stderr, "No such image '%s'.\n",imagefn);
	exit(1);
    }
    printf("Image '%s' is %d x %d\n", imagefn, uData->width, uData->height);

} // load_image



/*  IMPORTANT for OpenGL & GLSL : Know your version numbers!
static void printGLversion(void)
{   
    printf("GL Vendor    :'%s'.\n",glGetString(GL_VENDOR)) ;
    printf("GL Renderer  :'%s'.\n",glGetString(GL_RENDERER)) ;
    printf("GL Version   :'%s'.\n",glGetString(GL_VERSION)) ;
    printf("GLSL Version :'%s'.\n",glGetString(GL_SHADING_LANGUAGE_VERSION)) ;
    printf("GL Extensions:'%s'.\n",glGetString(GL_EXTENSIONS)) ;
} // printGLversion
*/



/***********************************************************
 * Name: initialise
 *
 * Arguments:
 *     ESContext *esContext - holds display/user data.
 *
 * Description: Main initialisation routine.
 *
 * Returns: void
 *
 ***********************************************************/
static void initialise(int argc, char **argv, ESContext *esContext)
{
    UserData *user = esContext->userData;

    // Set seed of random number generator using time().
    srand((unsigned int) time(NULL)) ;

    // Extract input parameters.
    parse(argc,argv,esContext) ;

    // Initialise the keyboard input.
    user->keyboard_fd = init_keyboard() ; 

    load_image(user) ;

    // Set up the exit function for exit(0) or the main return.
    atexit(exit_func) ;   

//    exit(0) ;   // For testing/development.

} // initialise




///
// Initialize the first basic shader and program object. 
//
static int init_shaders1(ESContext *esContext) {
    UserData *userData = esContext->userData;
    GLbyte vShaderStr[] =
        "attribute vec4 vPosition;                  \n"
        "void main()                                \n"
        "{                                          \n"
        "   gl_Position = vPosition;                \n"
        "}                                          \n";
    GLbyte fShaderStr[] =
        "precision mediump float;                   \n"
        "void main()                                \n"
        "{                                          \n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"   // Red
        "}                                          \n";

    // Store the program object
    userData->programObject = esLoadProgram((char *)vShaderStr,(char *)fShaderStr);

    return userData->programObject ;   // 0 = FALSE = Failure

} // init_shaders1





///
// Initialize the second shader and program object. 
//  Trying to load up a vertex colour. Working.
static int init_shaders2(ESContext *esContext) {
    UserData *user = esContext->userData;
    GLbyte vShaderStr[] =
        "attribute vec3 a_position;                   \n"
        "attribute vec3 a_colour;                     \n"
        "uniform   mat4 MVP;                          \n"
        "varying   vec3 v_colour;                     \n"
        "void main()                                  \n"
        "{                                            \n"
        "   v_colour = a_colour;                      \n"
        "   gl_Position = MVP * vec4(a_position,1.0); \n"
        "}                                            \n";

    GLbyte fShaderStr[] =
        "#version 100                                 \n"
        "uniform   vec3 u_colour;                     \n"
        "varying   vec3 v_colour;                     \n"
        "void main()                                  \n"
        "{                                            \n"
        "  gl_FragColor = vec4(v_colour, 1.0);        \n" 
        "}                                            \n";

    // Store the program object
    user->programObject = esLoadProgram((char *)vShaderStr,(char *)fShaderStr);

    // Get the attribute locations
    user->positionLoc = glGetAttribLocation( user->programObject, "a_position" );
    user->colourLoc = glGetAttribLocation( user->programObject, "a_colour" );
//    user->colourLoc = glGetUniformLocation( user->programObject, "u_colour" );

    return user->programObject ;   // 0 = FALSE = Failure

} // init_shaders2

/*
        "attribute vec3 a_normal;                     \n"
        "attribute vec2 a_texcoord;                   \n"
        "attribute vec3 a_colour;                     \n"
        "varying   vec2 v_texcoord;                   \n"
        "varying   vec3 v_colour;                     \n"
        "   v_texcoord  = a_texcoord;                 \n"

        "varying   vec3 v_colour;                     \n"
        "varying   vec2 v_texcoord;                   \n"
        "  gl_FragColor = vec4(v_colour, 1.0);        \n" 
*/



///
// Initialize the second shader and program object. 
//  Trying to load up a vertex image. Works with init_withoutVBOs().
static int init_shaders3(ESContext *esContext) {
    UserData *user = esContext->userData;
    GLbyte vShaderStr[] =
        "attribute vec3 a_position;                   \n"
        "attribute vec2 a_texcoord;                   \n"
        "uniform   mat4 MVP;                          \n"
        "varying   vec2 v_texcoord;                   \n"
        "void main()                                  \n"
        "{                                            \n"
        "   v_texcoord  = a_texcoord;                 \n"
        "   gl_Position = MVP * vec4(a_position,1.0); \n"
        "}                                            \n";
    GLbyte fShaderStr[] =
        "precision mediump float;                             \n"
        "varying   vec2 v_texcoord;                           \n"
        "uniform sampler2D s_texture;                         \n"
        "void main()                                          \n"
        "{                                                    \n"
        "   gl_FragColor = texture2D( s_texture, v_texcoord );\n"
        "}                                                    \n";

    // Store the program object
    user->programObject = esLoadProgram((char *)vShaderStr,(char *)fShaderStr);

    // Get the attribute locations
    user->positionLoc = glGetAttribLocation( user->programObject, "a_position" );
    user->texCoordLoc = glGetAttribLocation( user->programObject, "a_texcoord" );
   
    // Get the sampler location
    user->samplerLoc = glGetUniformLocation( user->programObject, "s_texture" );
    // Load the texture
    user->textureId = loadTexture2D(user->image, user->width, user->height);

    return user->programObject ;   // 0 = FALSE = Failure

} // init_shaders3







static int init_shaders(ESContext *esContext)
{
    UserData *user = esContext->userData;
    int ret = 0 ;

    switch ( user->routine ) {
        case 1 : // Triangle
            ret = init_shaders1(esContext) ;
            break ;
        case 2 : // Coloured Cube
            ret = init_shaders2(esContext) ;
            break ;
        case 3 : // Textured Cube
            ret = init_shaders3(esContext) ;
            break ;
        case 4 : // Coloured Sphere
            ret = init_shaders2(esContext) ;
            break ;
        default :
            ret = init_shaders1(esContext) ;
            break ;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);       // Black

    // Enable back face culling & depth testing. Working. 
    // Must use '| ES_WINDOW_DEPTH' with esCreateWindow().
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    return ret ;   // > 0 for success

} // init_shaders





// The vertex shaders require a single matrix 4x4 (x,y,z,w) called MVP
// to move and project the model vertex data every update interval.
// (x,y,z,w) is a homogeneous coordinate system.
// w=1 for position vector and w=0 for direction vector.
// This coordinate system allows for a Translation matrix.
//
// Model matrix = Translation matrix * Rotation matrix * Scale matrix.
//
// MVP = Projection matrix * Camera_View matrix * Model matrix.
//
// This is the main computation required by the CPU every update.
// In the GPU vertex shader every vertex point is multiplied by MVP to 
// move & project it into the clip/screen coordinates. 

static void Update_MVP(ESContext *esContext, float deltatime)
{
// Do not have the FAR_CLIP too large because of depth resolution.
#define NEAR_CLIP          0.1f    // Depth clipping must be >0.0  
#define FAR_CLIP         100.0f    // Depth clipping must be >0.0
#define FOV               45.0f    // Field of view in degrees.
#define CAMERA_DISTANCE    5.0f 
    UserData *user = esContext->userData;
    OBJECT_T *ob = &user->object[user->obj] ;
    float angle = user->count * 1.0 ;  // Rotate object?
    float ypos = user->count * 0.0 ;   // Move object slowly upwards.?

// Compute the Model matrix.
    esMatrixLoadIdentity(&ob->modelMat) ;
    esRotate(&ob->modelMat,angle,1.0f,1.0f,0.0f) ;
    esTranslate(&ob->modelMat,0.0f,ypos,0.0f) ;

// Compute the Camera View matrix.
// Camera position and direction looking at (0,0,0) from a set distance.
    esMatrixLoadIdentity(&ob->viewMat) ;
    esRotate(&ob->viewMat,180.0f,0.0f,1.0f,0.0f) ;
    esTranslate(&ob->viewMat,0.0f,0.0f,CAMERA_DISTANCE) ;

// Compute the Projection matrix.
    esMatrixLoadIdentity(&ob->projMat) ;
    esPerspective(&ob->projMat,FOV,user->aspect,NEAR_CLIP,FAR_CLIP) ;

// Multiple all together.
    esMatrixMultiply(&ob->mvpMat,&ob->modelMat,&ob->viewMat) ;
    esMatrixMultiply(&ob->mvpMat,&ob->mvpMat,&ob->projMat) ;

    glUniformMatrix4fv(ob->mvpId,1,GL_FALSE,&(ob->mvpMat.m[0][0])) ;

} // Update_MVP




static void Update(ESContext *esContext, float deltatime)
{
    UserData *user = esContext->userData;

    if ( user->nobjs ) {

      switch ( user->routine ) {
        case 1 : // Triangle
            break ;
        case 2 : // Coloured Cube
            Update_MVP(esContext,deltatime) ;
            break ;
        case 3 : // Textured Cube
            Update_MVP(esContext,deltatime) ;
            break ;
        case 4 : // Coloured Sphere
            Update_MVP(esContext,deltatime) ;
            break ;
        default :
            break ;
      }

    }

} // Update



///
// Draw a triangle using the shader pair created in init_shaders1().
//
static void Draw_Triangle(ESContext *esContext) {
    UserData *userData = esContext->userData;
    GLfloat vVertices[] = {0.0f,  0.5f, 0.0f,
                          -0.5f, -0.5f, 0.0f,
                           0.5f, -0.5f,  0.0f};

    // Use the program object
    glUseProgram(userData->programObject);

    // Load the vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);

    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

} // Draw_Triangle





///
// Draw a coloured object using the shader pair created in init_shaders2().
static void Draw_Coloured_Object(ESContext *esContext) {
    UserData *user = esContext->userData;
    OBJECT_T *ob = &user->object[0] ;

    // Use the program object
    glUseProgram(ob->program);

//    glUniform3f(user->colourLoc, 1.0f, 1.0f, 0.0f ) ;

// Working when using init_withoutVBOs(),
// but loads up vertex data from client memory each call!
//    glDrawElements(GL_TRIANGLES, ob->ni, GL_UNSIGNED_SHORT, &(ob->i[0]));

// Working when using init_withVBOs() which uses vertex data preloaded into GPU memory.
//    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[0]) ;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob->vboIds[4]) ;
    glDrawElements(GL_TRIANGLES, ob->ni, GL_UNSIGNED_SHORT, BUF_OFFSET(0));

} // Draw_Coloured_Object



///
// Draw a textured cube using the shader pair created in init_shaders3().
static void Draw_Textured_Cube(ESContext *esContext) {
    UserData *user = esContext->userData;
    OBJECT_T *ob = &user->object[0] ;
//    void *void0 = (void *) 0;

    // Use the program object
    glUseProgram(ob->program);

// Working when using init_withoutVBOs(),
// but loads up vertex data from client memory each call!
    glDrawElements(GL_TRIANGLES, ob->ni, GL_UNSIGNED_SHORT, &(ob->i[0]));

// Working when using init_withVBOs() which uses vertex data preloaded into GPU memory.
//    glBindBuffer(GL_ARRAY_BUFFER, ob->vboIds[0]) ;
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ob->vboIds[4]) ;
//    glDrawElements(GL_TRIANGLES, ob->ni, GL_UNSIGNED_SHORT, BUF_OFFSET(0));

} // Draw_Textured_Cube




static void Draw(ESContext *esContext)
{
    UserData *user = esContext->userData;

    // Set the viewport
    glViewport(0, 0, esContext->width, esContext->height);

    // Start with a clear screen
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    switch ( user->routine ) {
        case 1 :
            Draw_Triangle(esContext) ;
            break ;
        case 2 :
            Draw_Coloured_Object(esContext) ;
            break ;
        case 3 :
            Draw_Textured_Cube(esContext) ;
            break ;
        case 4 :
            Draw_Coloured_Object(esContext) ;
            break ;
        default :
            Draw_Triangle(esContext) ;
            break ;
    }

} // Draw





//==============================================================================

static int myMainLoop (ESContext *esContext)
{
    UserData *user = esContext->userData;
//    int i ;
    int iLimit = 10000000 ;   // While loop count limit control
    int iTimeLoop = 0 ;  
    double dKeyCheck = 0.0 ;       // enable for now
    double dPeriod = 0.0 ;         // While loop elapsed time control
    double deltaTime = 0.0 ;
    double cur_etime = 0.0 ;
//    struct timespec pause = { 1 , 0 } ;  // 1.0s


    // Setup the model world
//    init_model_proj(esContext);

    esRegisterUpdateFunc(esContextp, Update);
    esRegisterDrawFunc(esContextp, Draw);

    // Period in whole microseconds.
    dPeriod = (double) floor(user->period * MICRO + 0.5) ;
    if ( user->keyboard_fd >= 0 ) printf("Press ESC to quit. :\n") ;

    // Loop until count limit or timeout occurs.
    resettimer(0) ;
    user->etime = uelapsedtime(0) ;
    while ( !user->toexit && ++user->count <= iLimit )
    {
        cur_etime = uelapsedtime(0) ;
        deltaTime = (float) (cur_etime - user->etime) ;
        user->etime = cur_etime ;
       
        if (esContext->updateFunc != NULL)
            esContext->updateFunc(esContext, (float) deltaTime);
        if (esContext->drawFunc != NULL)
            esContext->drawFunc(esContext);

        eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);
  
        if ( ++iTimeLoop == 30 ) {  // 1 loop ~16ms, 30 ~= 480ms
            if ( user->etime > dPeriod ) user->toexit = 1 ;
            iTimeLoop = 0 ;
//            printf(".") ; fflush(NULL) ;
            if ( !user->toexit && user->etime > dKeyCheck ) {
                if ( getkeycode(user->keyboard_fd) == 1 ) user->toexit = 1 ;
                dKeyCheck = user->etime + MICRO ; // Check again in a second.
            }
        }
//        nanosleep(&pause,NULL) ;
    }
    user->etime = uelapsedtime(0) ;
    --user->count ;
    printf("\nStopped!\n") ;

    double et = user->etime / MICRO ;
    printf("Time taken for %d loops : %.3fs, %.3fms/frame, %.1fHz\n",
           user->count,et,et*1000.0/user->count,user->count/et) ;

    return 0;   

} // myMainLoop








int main(int argc, char **argv) {
    UserData *user_p = &userData ;

    // Clear UserData memory.
    memset( user_p , 0, sizeof( UserData ) );

    esInitContext(esContextp);
    esContext.userData = user_p;

    // General initialise 
    initialise(argc,argv,esContextp) ;

    // IMPORTANT : Use  '| ES_WINDOW_ALPHA | ES_WINDOW_DEPTH' flags.
    assert( esCreateWindow(esContextp, "Hello World", 0, 0, ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH) == GL_TRUE ) ;
    printf("Screen size : (%d,%d).\n",esContext.width,esContext.height) ;
    user_p->aspect = esContext.width / esContext.height ;

    if ( !init_shaders(esContextp) ) return 0; // Will run exit_func()

    initialise_objects(esContextp) ;  // After shaders set up.

    myMainLoop(esContextp); 

//    printGLversion() ;

    return 0 ;   // Will run exit_func()

} // main


