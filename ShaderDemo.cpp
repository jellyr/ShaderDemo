// ShaderDemo.cpp : This is it. Created by eiffie GPLv3
// What is it? Runs simple webgl scripts from ShaderToy under win 32 api.
// Why? I wanted to run the games offline/native at fullscreen/speed.
// Usage: Create a textfile and type [bufA] then newline. Copy in code for buffer A
// type [image] on a line by itself then copy in the code from Image tab.
// Optionally add a line for [sound] and add the code from the Sound tab.
// Save the file and drag it to this program's icon. Buffer A=iChannel0 and keyboard=iChannel1
// Press "escape" to quit and "backspace" to reset iFrame.
// You need these standard headers and GL.h (I seem to have an old version!)
// The (VS) project should use single byte chars.
#pragma comment( lib, "OpenGL32.lib" )
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "GL.h"

#pragma warning(disable: 4244) // converting from double to int, we know 
#pragma warning(disable: 4267) // more conversions
#pragma warning(disable: 4996) // depricated
#pragma warning(disable: 4800) // forcing to bool
#pragma warning(disable: 4312) // casting to greater size

#define ADD_SOUND
#ifdef ADD_SOUND 
#pragma comment( lib, "WinMM.lib" ) //for PlaySound
//WAVE FILE FUNCTIONS
typedef struct tWAVEDATAFORMAT //this is the format of a .wav file that can be created and played in mem
{
	DWORD		dwRIFF,dwLen,dwWAVE,dwfmt,dwFmtLen;
    WORD        wFormatTag,nChannels;          
    DWORD       nSamplesPerSec,nAvgBytesPerSec;   
    WORD        nBlockAlign,wBitsPerSample;  
	DWORD		dwdata,dwDataLen;
	short		data[1];
} WAVEDATAFORMAT;
//create a Wave file in memory with these attributes (you then fill in the data array)
WAVEDATAFORMAT *createWave(int SampleRate, int Channels, int BitsPerSample, int Bytes){
	WAVEDATAFORMAT *wff=(WAVEDATAFORMAT *)malloc(sizeof(WAVEDATAFORMAT)+Bytes);
	if(wff){
	wff->dwRIFF=MAKEFOURCC('R','I','F','F');
	wff->dwLen=Bytes+36;
	wff->dwWAVE=MAKEFOURCC('W','A','V','E');
	wff->dwfmt=MAKEFOURCC('f','m','t',' ');
	wff->dwFmtLen=16;
	wff->wFormatTag=1;
	wff->nChannels=Channels;
	wff->nSamplesPerSec=SampleRate;
	wff->nAvgBytesPerSec=SampleRate*Channels*BitsPerSample/8;
	wff->nBlockAlign=Channels*BitsPerSample/8;
	wff->wBitsPerSample=BitsPerSample;
	wff->dwdata=MAKEFOURCC('d','a','t','a');
	wff->dwDataLen=Bytes;}
	return wff;//what is left is for you to fill in the wff->data and free this when done
}
void StopWave(){PlaySound(NULL,NULL,0);}//stops any playing wave files
DWORD sndflags(bool loop){DWORD flg=SND_ASYNC;if(loop)flg=flg|SND_LOOP;return flg;}
void PlayWave(WAVEDATAFORMAT *wdf){PlaySound((LPCSTR)(void *)wdf,NULL,sndflags(true) | SND_MEMORY);}
float sgn(float t){return (t<0.0f?-1.0f:1.0f);}
#endif

char *bufferA=NULL,*image=NULL,*sound=NULL;
char VSscript[]="void main() {gl_Position = gl_Vertex;}";
char fsh[]="uniform sampler2D Zbuf,Ztex;\n\
		   uniform float Zuni[16];\n\
		   #define iResolution vec4(Zuni[0],Zuni[1],0.0,0.0)\n\
		   #define iGlobalTime Zuni[2]\n\
		   #define iMouse vec4(Zuni[3],Zuni[4],Zuni[5],Zuni[6])\n\
		   #define iFrame int(Zuni[7])\n\
		   #define iTimeDelta Zuni[8]\n\
		   #define iDate vec4(Zuni[9],Zuni[10],Zuni[11],Zuni[12])\n\
		   #define iChannel0 Zbuf\n\
		   #define iChannel1 Ztex\n\
		   vec2 iChannelResolution[2];\n\
		   void mainImage(out vec4, in vec2);\n\
		   void main(){iChannelResolution[0]=iResolution.xy;\n\
		   iChannelResolution[1]=vec2(256.0,2.0);\n\
		   mainImage(gl_FragColor,gl_FragCoord.xy);}\n%s";

bool loadShaders(char *fname){
	FILE *fp;
	char buff[512],*txt,*buf;
	fp = fopen (fname, "rb");
	if (!fp) return false;
	fgets (buff, sizeof (buff), fp); //Read whole line
	if(memcmp(buff,"[bufA]",6)){fclose(fp);return false;}
	bufferA=(char *)malloc(65536);
	image=(char *)malloc(65536);
	sound=(char *)malloc(65536);
	bufferA[0]=0;image[0]=0;sound[0]=0;
	txt=bufferA;
	int i=0;
	while (!feof(fp)){//load [bufA] and [image]
		buf=fgets (buff, sizeof (buff), fp); //Read whole line
		if(buf){
			if(!memcmp(buff,"[image]",7)){
				txt=image;i=0;
			}else if(!memcmp(buff,"[sound]",7)){
				txt=sound;i=0;
			}else strcat(txt,buff);
#ifdef _DEBUG
			if(i==127-15){//put error line# here
				int h=0;
			}
			i++;
#endif
		}
	}
	fclose(fp);
	return (image[0]!=0);
}
typedef void (__stdcall * PFNGLACTIVETEXTUREPROC) (GLenum texunit);
//typedef void (__stdcall * PFNGLTEXIMAGE3DEXTPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
typedef void (__stdcall * PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (__stdcall * PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (__stdcall * PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (__stdcall * PFNGLCREATESHADERPROC) (GLenum type);
typedef void (__stdcall * PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (__stdcall * PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (__stdcall * PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (__stdcall * PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (__stdcall * PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (__stdcall * PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const char** strings, const GLint* lengths);
typedef void (__stdcall * PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint* param);
typedef void (__stdcall * PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint* param);
typedef void (__stdcall * PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void (__stdcall * PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef GLint (__stdcall * PFNGLGETUNIFORMLOCATION) (GLuint program, const char *name);
typedef void (__stdcall * PFNGLUNIFORMFVPROC) (GLint location, GLsizei count, const GLfloat* value);
typedef void (__stdcall * PFNGLUNIFORMIVPROC) (GLint location, GLsizei count, const GLint* value);
typedef void (__stdcall * PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint* framebuffers);
typedef void (__stdcall * PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (__stdcall * PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint textur, GLint level);
typedef void (__stdcall * PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint* framebuffers);
typedef GLenum (__stdcall * PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_TEXTURE_3D_EXT 0x806F
#define GL_FRAMEBUFFER 0x8D40
#define GL_TEXTURE0 0x84C0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RGBA32F 0x8814
//#define GL_RGB32F 0x8815

PFNGLACTIVETEXTUREPROC glActiveTexture=NULL;
//PFNGLTEXIMAGE3DEXTPROC glTexImage3D=NULL;
PFNGLDELETESHADERPROC glDeleteShader=NULL; 
PFNGLDELETEPROGRAMPROC glDeleteProgram=NULL; 
PFNGLCREATESHADERPROC glCreateShader=NULL;
PFNGLSHADERSOURCEPROC glShaderSource=NULL;
PFNGLCOMPILESHADERPROC glCompileShader=NULL;
PFNGLATTACHSHADERPROC glAttachShader=NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram=NULL;
PFNGLLINKPROGRAMPROC glLinkProgram=NULL;
PFNGLUSEPROGRAMPROC glUseProgram=NULL;
PFNGLDETACHSHADERPROC glDetachShader=NULL;
PFNGLGETSHADERIVPROC glGetShaderiv=NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv=NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog=NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog=NULL;
PFNGLGETUNIFORMLOCATION glGetUniformLocation=NULL;
PFNGLUNIFORMFVPROC glUniform1fv=NULL;
PFNGLUNIFORMIVPROC glUniform1iv=NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers=NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers=NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer=NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D=NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus=NULL;

bool LoadShaderProcs(){//call after starting OpenGL
	glActiveTexture=(PFNGLACTIVETEXTUREPROC)wglGetProcAddress((LPCSTR)"glActiveTexture");
	//glTexImage3D=(PFNGLTEXIMAGE3DEXTPROC)wglGetProcAddress((LPCSTR)"glTexImage3DEXT");
	glDeleteShader=(PFNGLDELETESHADERPROC)wglGetProcAddress((LPCSTR)"glDeleteShader"); 
	glDeleteProgram=(PFNGLDELETEPROGRAMPROC)wglGetProcAddress((LPCSTR)"glDeleteProgram"); 
	glCreateShader=(PFNGLCREATESHADERPROC)wglGetProcAddress((LPCSTR)"glCreateShader");
	glShaderSource=(PFNGLSHADERSOURCEPROC)wglGetProcAddress((LPCSTR)"glShaderSource");
	glCompileShader=(PFNGLCOMPILESHADERPROC)wglGetProcAddress((LPCSTR)"glCompileShader");
	glCreateProgram=(PFNGLCREATEPROGRAMPROC)wglGetProcAddress((LPCSTR)"glCreateProgram");
	glAttachShader=(PFNGLATTACHSHADERPROC)wglGetProcAddress((LPCSTR)"glAttachShader");
	glLinkProgram=(PFNGLLINKPROGRAMPROC)wglGetProcAddress((LPCSTR)"glLinkProgram");
	glUseProgram=(PFNGLUSEPROGRAMPROC)wglGetProcAddress((LPCSTR)"glUseProgram");
	glDetachShader=(PFNGLDETACHSHADERPROC)wglGetProcAddress((LPCSTR)"glDetachShader");
	glGetShaderiv=(PFNGLGETSHADERIVPROC)wglGetProcAddress((LPCSTR)"glGetShaderiv");
	glGetProgramiv=(PFNGLGETPROGRAMIVPROC)wglGetProcAddress((LPCSTR)"glGetProgramiv");
	glGetProgramInfoLog=(PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress((LPCSTR)"glGetProgramInfoLog");
	glGetShaderInfoLog=(PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress((LPCSTR)"glGetShaderInfoLog");
	glGetUniformLocation=(PFNGLGETUNIFORMLOCATION)wglGetProcAddress((LPCSTR)"glGetUniformLocation");
	glUniform1fv=(PFNGLUNIFORMFVPROC)wglGetProcAddress((LPCSTR)"glUniform1fv");
	glUniform1iv=(PFNGLUNIFORMIVPROC)wglGetProcAddress((LPCSTR)"glUniform1iv");
	glDeleteFramebuffers=(PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress((LPCSTR)"glDeleteFramebuffers");
	glGenFramebuffers=(PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress((LPCSTR)"glGenFramebuffers");
	glBindFramebuffer=(PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress((LPCSTR)"glBindFramebuffer");
	glFramebufferTexture2D=(PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress((LPCSTR)"glFramebufferTexture2D");
	glCheckFramebufferStatus=(PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress((LPCSTR)"glCheckFramebufferStatus");
	bool bFrameBuffers=(glDeleteFramebuffers&&glGenFramebuffers&&glBindFramebuffer&&glFramebufferTexture2D&&glCheckFramebufferStatus);
	if(!(bFrameBuffers&&glCreateShader&&glShaderSource&&glCompileShader&&glCreateProgram&&glAttachShader&&glLinkProgram&&
		glUseProgram&&glDetachShader&&glGetShaderiv&&glGetProgramiv&&glGetProgramInfoLog&&glGetShaderInfoLog)){
		MessageBox(NULL,"Could not get OpenGL process addresses. Expecting 2.0 functionality.","Shader Error",MB_OK);
		return false;
	}
	return true;
}
bool createprogram(char *VSscript, char *FSscript, GLuint &P, GLuint &VS, GLuint &FS){
	if(!(VSscript && FSscript))return false;
	if(!(VSscript[0] && FSscript[0]))return false;
	P=glCreateProgram();
	VS=glCreateShader(GL_VERTEX_SHADER);
	FS=glCreateShader(GL_FRAGMENT_SHADER);
	GLint iStatus=0;
	const char *fs=VSscript;
	glShaderSource(VS, 1, &fs, NULL);
	glCompileShader(VS);
	glGetShaderiv(VS,GL_COMPILE_STATUS,&iStatus);
	if(!iStatus){
		char *log=(char *)malloc(1024);
		if(!log)return false;
		glGetShaderInfoLog(VS, 1024, &iStatus, log);
		MessageBox(NULL,log,"vertex error",MB_OK);
		free(log);
		return false;
	}
	fs=FSscript;
	glShaderSource(FS, 1, &fs, NULL);
	glCompileShader(FS);
	glGetShaderiv(FS,GL_COMPILE_STATUS,&iStatus);
	if(!iStatus){
		char *log=(char *)malloc(1024);
		if(!log)return false;
		glGetShaderInfoLog(FS, 1024, &iStatus, log);
		MessageBox(NULL,log,"fragment error",MB_OK);
		free(log);
		return false;
	}
	glAttachShader(P,VS);
	glAttachShader(P,FS);
	glLinkProgram(P);
	glGetProgramiv(P,GL_LINK_STATUS,&iStatus);
	if(!iStatus){
		char *log=(char *)malloc(1024);
		if(!log)return false;
		glGetProgramInfoLog(P, 1024, &iStatus, log);
		glDetachShader(P,VS);
		glDetachShader(P,FS);
		MessageBox(NULL,log,"link error",MB_OK);
		free(log);
		return false;
	}
	return true;
}

void setkbtexture(void *bits, GLuint ztexture){//keyboard texture
	glActiveTexture(GL_TEXTURE0+1);							// Texture #1
	glBindTexture(GL_TEXTURE_2D, ztexture);					// Tell GL we are using it
	glTexImage2D(GL_TEXTURE_2D,0,GL_RED,256,1,0,GL_RED,GL_UNSIGNED_BYTE,bits);//should be 256x2
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glActiveTexture(GL_TEXTURE0);
}
void SetTexParams(int width, int height){//render textures
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL); 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
}

BYTE bytKeys[256];
int iMouse=0,iFrame=0;
bool bQuit=false,bKeys=true;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
	case WM_KEYDOWN:
		bKeys=true;
		if(wParam<256)bytKeys[wParam]=255;
		if(wParam==27)bQuit=true;
		if(wParam==8)iFrame=0;
		break;
	case WM_KEYUP:bKeys=true;if(wParam<256)bytKeys[wParam]=0;break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:iMouse=wParam;break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:iMouse=0;break;
	case WM_DESTROY:
		bQuit=true;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool handleinput(bool wait = false){//handles one message, can wait for it
	MSG msg;
	if (wait ? GetMessage(&msg, NULL, 0, 0) : PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
   		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return true;
	}
	return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(!argv[1])return -1;
	if(argv[1][0]==0)return -1;
	if(!loadShaders(argv[1]))return -1;
	//these describe the window
#ifdef _DEBUG
	int maxx=640,maxy=360;
#else
	int maxx=0,maxy=0;
#endif
	bool MODE_FULLSCREEN=(maxx==0);

	//go fullscreen if needed
	int BW=GetSystemMetrics(SM_CXFIXEDFRAME)*2;
	int BH=GetSystemMetrics(SM_CYFIXEDFRAME)*2+GetSystemMetrics(SM_CYCAPTION);
	DEVMODE dmSave;
	dmSave.dmSize=sizeof(DEVMODE);
	BOOL ret=EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dmSave);
	int width=(ret?dmSave.dmPelsWidth:640)-((MODE_FULLSCREEN)?0:BW);
	int height=(ret?dmSave.dmPelsHeight:480)-((MODE_FULLSCREEN)?0:BH);
	width=min((maxx>0?maxx:width),width);
	height=min((maxy>0?maxy:height),height);

	//create a window
	HWND hWnd=NULL;HDC hdc=NULL;HGLRC hrc;//window handles
	WNDCLASSEX wcex;ZeroMemory(&wcex,sizeof(wcex));wcex.cbSize=sizeof(WNDCLASSEX);
	wcex.style=CS_HREDRAW|CS_VREDRAW;wcex.lpfnWndProc=(WNDPROC)WndProc;
	wcex.hInstance=GetModuleHandle(NULL);wcex.lpszClassName="ShaderDemo";
	if(!RegisterClassEx(&wcex))return -1;
	DWORD dwStyle;
	if(MODE_FULLSCREEN){//fullscreen
		dwStyle=WS_POPUP;
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth=width;						// Selected Screen Width
		dmScreenSettings.dmPelsHeight=height;					// Selected Screen Height
		dmScreenSettings.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT;
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return -1;
	}else {dwStyle=WS_CAPTION | WS_BORDER;}
	hWnd=CreateWindow("ShaderDemo","ShaderDemo",WS_VISIBLE|WS_CLIPCHILDREN|dwStyle,CW_USEDEFAULT,CW_USEDEFAULT,
		width+(MODE_FULLSCREEN?0:BW),height+(MODE_FULLSCREEN?0:BH),NULL,NULL,wcex.hInstance,NULL);
	if(!hWnd)return -1;
	ShowWindow(hWnd,SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	//ready opengl to use the window
	PIXELFORMATDESCRIPTOR pfd;ZeroMemory(&pfd,sizeof(pfd));//set the pixel format for the DC
	pfd.nSize = sizeof( pfd );pfd.nVersion = 1;pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;pfd.cColorBits = 24;pfd.iLayerType = PFD_MAIN_PLANE;
	if(!(hdc=GetDC(hWnd)))return -1;			// Does Our Window Have A Device Context?
	unsigned int PixelFormat;  //we have a context now we need to make sure it handles our pixel format
	if(!(PixelFormat=ChoosePixelFormat(hdc,&pfd)))return -1; // Did Windows Find A Matching Pixel Format?
	if(!SetPixelFormat(hdc,PixelFormat,&pfd))return -1;		// Are We Able To Set The Pixel Format of Our Window?
	if(!(hrc=wglCreateContext(hdc)))return -1;				// Are We Able To Get A Rendering Context?
	if(!wglMakeCurrent(hdc,hrc))return -1;					// Try To Activate The Rendering Context

	//get OpenGL2 procs & buffers
	if(!LoadShaderProcs())return -1;						// Get Addresses to 2.0 functionality
	GLuint texture,fbo[2],rttex[2];
	glGenFramebuffers(2, fbo);								// Rendering to buffers
	glGenTextures(2, rttex);								// Create named textures to render to
	glGenTextures(1, &texture);								// For keyboard buffer
	glActiveTexture(GL_TEXTURE0);							// Render to texture 0
	for(int i=0;i<2;i++){
		glBindTexture(GL_TEXTURE_2D, rttex[i]);					// Bind to the render texture
		SetTexParams(width,height);								// Create it and set params
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);				//Bind to the frame buffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rttex[i], 0);  //Attach texture  
		GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER); 
		if (status != GL_FRAMEBUFFER_COMPLETE) {glBindFramebuffer(GL_FRAMEBUFFER, 0);return -1;}//Bail
	}
	glViewport(0,0,width,height);// Set the Viewport
	
	//create the programs bufferA & image
	GLuint P_A, VS_A, FS_A, P_I, VS_I, FS_I;
	GLint zBufA, zTexA, zUniA, zBufI, zTexI, zUniI;
	char *FSscript=(char *)malloc(65536); //max text for scripts
	sprintf(FSscript,fsh,bufferA);
	if(!createprogram(VSscript, FSscript, P_A, VS_A, FS_A)){free(FSscript);return -1;}
	glUseProgram(P_A);
	zUniA=glGetUniformLocation(P_A,"Zuni");//set uniforms
	zBufA=glGetUniformLocation(P_A,"Zbuf");
	zTexA=glGetUniformLocation(P_A,"Ztex");
	sprintf(FSscript,fsh,image);
	if(!createprogram(VSscript, FSscript, P_I, VS_I, FS_I)){free(FSscript);return -1;}
	glUseProgram(P_I);
	zUniI=glGetUniformLocation(P_I,"Zuni");
	zBufI=glGetUniformLocation(P_I,"Zbuf");
	zTexI=glGetUniformLocation(P_I,"Ztex");
#ifdef ADD_SOUND
	WAVEDATAFORMAT *wdf=NULL;
	if(sound[0]!=0){
		char fssh[]="uniform float Zuni[2];\nvec2 mainSound(in float);\n\
		   void main(){float t=floor(gl_FragCoord.y)*Zuni[0]+floor(gl_FragCoord.x);\n\
		   vec2 v1=mainSound(t*2.0/Zuni[1]),v2=mainSound((t*2.0+1.0)/Zuni[1]);\n\
		   gl_FragColor=clamp(vec4(v1,v2),-1.0,1.0);}\n%s";
		GLuint P_S, VS_S, FS_S;//create the program and shaders
		sprintf(FSscript,fssh,sound);
		if(createprogram(VSscript, FSscript, P_S, VS_S, FS_S)){
			glBindFramebuffer(GL_FRAMEBUFFER, fbo[1]); 
			glUseProgram(P_S);
			GLint zUniS=glGetUniformLocation(P_S,"Zuni");//get uniform location
			#define SAMPLE_RATE 22050
			int bufSamps=width*height*4;//2 channels x 2 samps per pixel
			wdf=createWave(SAMPLE_RATE, 2, 16, bufSamps*sizeof(short));
			if(wdf){
				float *dat=(float *)malloc(sizeof(float)*bufSamps);
				if(dat){
					float u[2];u[0]=(float)width;u[1]=(float)SAMPLE_RATE;//sample rate
					glUniform1fv(zUniS,2,u);
					glRects(-1,-1,1,1);
					glReadPixels(0,0,width,height,GL_RGBA,GL_FLOAT,(void *)dat);
					for(int i=0;i<bufSamps;i++){
						float f=dat[i]*32766.0f;
						wdf->data[i]=(short)(f+sgn(f)*0.5f);
					}
					free(dat);
				}
			}
			glDetachShader(P_S,FS_S);glDeleteShader(FS_S);
			glDetachShader(P_S,VS_S);glDeleteShader(VS_S);
			glDeleteProgram(P_S);
		}
		if(wdf)PlayWave(wdf);//could be garbage in no dat!
	}
#endif
	free(FSscript);free(bufferA);free(image);free(sound);
 
	//now the demo loop
#define ARRAY_SIZE 16
	int hi=0,lastX=0,lastY=0,iBuf=0;
	const int ZERO=0,ONE=1;
	float u[ARRAY_SIZE]; for(int i=0;i<ARRAY_SIZE;i++)u[i]=0.0;
	u[0]=(float)width;//setting up uniforms
	u[1]=(float)height;
	DWORD iStartTime=GetTickCount(),lastTime=iStartTime;
	while(!bQuit){//press esc to quit
		while(handleinput(false)){}//read windows messages until there are none
		DWORD now=GetTickCount();
		DWORD s=max(1,int(20-(now-lastTime)));
		Sleep(s);
		u[2]=(float)(now-iStartTime)/1000.0;
		if(iMouse>0){
			POINT p;GetCursorPos(&p);ScreenToClient(hWnd,&p);p.y=height-p.y;
			u[3]=(float)p.x;
			u[4]=(float)p.y;
			if(lastX<=0){lastX=p.x;lastY=p.y;}
		}else if(lastX>0){lastX=-lastX;lastY=-lastY;}
		u[5]=(float)lastX;
		u[6]=(float)lastY;
		u[7]=((float)iFrame);
		u[8]=(float)(now-lastTime)/1000.0;
		u[12]=now/1000.0;
		
		if(bKeys){setkbtexture(bytKeys, texture);bKeys=false;} //if key change rebind texture
		glBindTexture(GL_TEXTURE_2D, rttex[iBuf]); //ping pong buffers to eliminate feedback
		iBuf=(iBuf+1)%2;
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[iBuf]); //buffer A
		glUseProgram(P_A);	
		glUniform1fv(zUniA,ARRAY_SIZE,u);
		glUniform1iv(zBufA,1,&ZERO);
		glUniform1iv(zTexA,1,&ONE);
		glRects(-1,-1,1,1);
		glBindFramebuffer(GL_FRAMEBUFFER, 0); //image
		glUseProgram(P_I); 
		glUniform1fv(zUniI,ARRAY_SIZE,u);
		glUniform1iv(zBufI,1,&ZERO);
		glUniform1iv(zTexI,1,&ONE);
		glRects(-1,-1,1,1);
		iFrame+=1;
		lastTime=now;
		SwapBuffers( hdc );
	}

	//clean this mess up
#ifdef ADD_SOUND
	if(wdf){StopWave();free(wdf);}
#endif
	glDetachShader(P_A,FS_A);glDeleteShader(FS_A);
	glDetachShader(P_A,VS_A);glDeleteShader(VS_A);
	glDeleteProgram(P_A);
	glDetachShader(P_I,FS_I);glDeleteShader(FS_I);
	glDetachShader(P_I,VS_I);glDeleteShader(VS_I);
	glDeleteProgram(P_I);
	glDeleteTextures(1,&texture);
	glDeleteTextures(2,rttex);
	glDeleteFramebuffers(2,fbo);
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(hrc);
	handleinput(true);
	DestroyWindow(hWnd);
	if(MODE_FULLSCREEN)ChangeDisplaySettings(&dmSave,CDS_RESET);
	return 0;
}

