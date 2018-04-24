#include <windows.h>
#include <float.h>
#include <math.h>
#include <GL/glut.h>
#include <iostream>

#define PI 3.1415926536

struct octNode{
	float rgb[3];
	octNode* nodes[8];
	float minX, maxX, minY, maxY, minZ, maxZ;
};

struct vec3{
	float x, y, z;
	vec3 operator+(int i){
		vec3 v = {x + i, y + i, z + i};
		return v;
	}
	vec3 operator+(vec3 v){
		vec3 nv = {x + v.x, y + v.y, z + v.z};
		return nv;
	}
	vec3 operator-(){
		vec3 v = {-x, -y, -z};
		return v;
	}
	vec3 operator-(float d){
		vec3 v = {x - d, y - d, z - d};
		return v;
	}
	vec3 operator-(vec3 v){
		vec3 nv = {x - v.x, y - v.y, z - v.z};
		return nv;
	}
	vec3 operator*(float d) const{
		vec3 v = {x * d, y * d, z * d};
		return v;
	}
	vec3 operator*(vec3 v){
		vec3 nv = {x * v.x, y * v.y, z * v.z};
		return nv;
	}
	vec3 operator/(float d){
		vec3 v = {x / d, y / d, z / d};
		return v;
	}
	float dot(vec3 v){
		return (x * v.x) + (y * v.y) + (z * v.z);
	}
	vec3 root(){
		vec3 v = {(float) sqrt(x), (float) sqrt(y), (float) sqrt(z)};
		return v;
	}
	vec3 sqr(){
		vec3 v = {x * x, y * y, z * z};
		return v;
	}
};
vec3 operator+(int i, vec3 v){
	return v + i;
}
vec3 operator*(float d, vec3 v){
	return v * d;
}

vec3 viewpoint = {0.0, 1.0, 5.0};
vec3 vp_u = {-1.0, 0.0, 0.0};
vec3 vp_v = {0.0, 1.0, 0.0};
vec3 vp_w = {0.0, 0.0, -1.0};
float vp_d = 1.0;

int angle = 180;
int height = 100;
int width = 100;

void deleteOctNode(octNode* node){
	for(int i = 0; i < 8; i++){
		if(node->nodes[i] != NULL){
			deleteOctNode(node->nodes[i]);
		}
	}
	delete[] node;
}

float length(const vec3 &v){
	return sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

vec3 normalize(const vec3 &v){
	float n = length(v);
	if(n == 0.0){
		n = FLT_MIN;
	}
	return v * (1.0 / n);
}

float* displayBuffer = NULL;

void setPixel(int x, int y, float r, float g, float b){
	float* pixel = &displayBuffer[(y * width * 3) + (x * 3)];
	pixel[0] = r;
	pixel[1] = g;
	pixel[2] = b;
}

void displayFunc(){
	glClear(GL_COLOR_BUFFER_BIT);

	vec3 origin, dir;
	float* pixel;
	for(int y = 0; y < height; y++){
		for(int x = 0; x < width; x++){
			origin = viewpoint + (vp_u * ((1.0 / width) * (x + 0.5) - 0.5)) + (vp_v * ((1.0 / height) * (y + 0.5) - 0.5)) + (vp_w * vp_d);
			dir = normalize(origin - viewpoint);
			setPixel(x, y, 0.0, 0.0, 0.0);

			vec3 cutoff = origin + (100 * dir);
			float u = origin.y / (origin.y - cutoff.y);
			if(u >= 0){
				float hitX = origin.x - ((origin.x - cutoff.x) * u);
				float hitZ = origin.z - ((origin.z - cutoff.z) * u);
				if(hitX >= -1 && hitX <= 1 && hitZ >= -1 && hitZ <= 1){
					setPixel(x, y, 1.0, 0.0, 0.0);
				}
			}
		}
	}

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, displayBuffer);
	glFlush();
	glutSwapBuffers();
}

void moveCamera(float x, float y, float z){
	viewpoint.x += x;
	viewpoint.y += y;
	viewpoint.z += z;
}

void rotateCamera(int change){
	if(angle <= 0){
		angle = 360;
	}
	else if(angle >= 360){
		angle = 0;
	}
	angle += change;
	double radAngle = angle * PI / 180.0;
	vp_u.x = cos(radAngle);
	vp_u.z = sin(radAngle);
	vp_w.x = -sin(radAngle);
	vp_w.z = cos(radAngle);
}

void keyboardFunc(unsigned char key, int x, int y){
	switch(key){
		case 'w':
		case 'W':
			moveCamera(0.0, 0.0, -0.25);
			break;
		case 's':
		case 'S':
			moveCamera(0.0, 0.0, 0.25);
			break;
		case 'a':
		case 'A':
			moveCamera(0.25, 0.0, 0.0);
			break;
		case 'd':
		case 'D':
			moveCamera(-0.25, 0.0, 0.0);
			break;
		case 'r':
		case 'R':
			moveCamera(0.0, 0.25, 0.0);
			break;
		case 'f':
		case 'F':
			moveCamera(0.0, -0.25, 0.0);
			break;
		case 'q':
		case 'Q':
			rotateCamera(2);
			break;
		case 'e':
		case 'E':
			rotateCamera(-2);
			break;
	}
	glutPostRedisplay();
}

void reshapeFunc(int newWidth, int newHeight){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, newWidth, 0.0, newHeight);

	delete[] displayBuffer;
	displayBuffer = new float[newWidth * newHeight * 3];
	for(int i = 0; i < newWidth * newHeight * 3; i++){
		displayBuffer[i] = 0;
	}
	width = newWidth;
	height = newHeight;

	glClear(GL_COLOR_BUFFER_BIT);
}

int main(int argc, char** argv){
	//octNode temp = octNode();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);

	glClearColor(0.0, 0.0, 0.0, 0.0);

	reshapeFunc(width, height);

	glutCreateWindow("CS 657 Final Project");

	glutDisplayFunc(displayFunc);
	glutKeyboardFunc(keyboardFunc);
	glutReshapeFunc(reshapeFunc);

	glutMainLoop();
}
