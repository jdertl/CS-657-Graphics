#include <windows.h>
#include <float.h>
#include <math.h>
#include <GL/glut.h>
#include <iostream>

#define PI 3.1415926536

using namespace std;

struct octNode{
	bool term;
	float rgb[3];
	octNode* nodes[8];
};
struct octree{
	octNode* root;
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

int maxDepth = 100;

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

octree* tree;
float* displayBuffer = NULL;

void setPixel(int x, int y, float r, float g, float b){
	float* pixel = &displayBuffer[(y * width * 3) + (x * 3)];
	pixel[0] = r;
	pixel[1] = g;
	pixel[2] = b;
}

bool finalProcess(octNode* node, int x, int y){
	setPixel(x, y, node->rgb[0], node->rgb[1], node->rgb[2]);
}

int firstProcess(double tMinX, double tMinY, double tMinZ, double tMaxX, double tMaxY, double tMaxZ){
	unsigned char index = 0;
	if(tMinX > tMinY){
		if(tMinX > tMinZ){
			if(tMaxY < tMinX){
				index |= 2;
			}
			if(tMaxZ < tMinX){
				index |= 1;
			}
			return index;
		}
	}
	else if(tMinY > tMinZ){
		if(tMaxX < tMinY){
			index |= 4;
		}
		if(tMaxZ < tMinY){
			index |= 1;
		}
		return index;
	}
	if(tMaxX < tMinZ){
		index |= 4;
	}
	if(tMaxY < tMinZ){
		index |= 2;
	}
	return index;
}

int nextProcess(double tX, int nextX, double tY, int nextY, double tZ, int nextZ){
	if(tX < tY){
		if(tX < tZ){
			return nextX;
		}
	}
	else if(tY < tZ){
		return nextY;
	}
	return nextZ;
}

bool subProcess(double tMinX, double tMinY, double tMinZ, double tMaxX, double tMaxY, double tMaxZ, int adjust, octNode* node, int depth, int x, int y){
	if(node && tMaxX >= 0 && tMaxY >= 0 && tMaxZ >= 0){
		if(depth == maxDepth || node->term){
			return finalProcess(node, x, y);
		}
		double tMidX = (tMinX + tMaxX) * 0.5;
		double tMidY = (tMinY + tMaxY) * 0.5;
		double tMidZ = (tMinZ + tMaxZ) * 0.5;
		int curIndex = firstProcess(tMinX, tMinY, tMinZ, tMidX, tMidY, tMidZ);
		bool pass = false;
		while(curIndex < 8){
			switch(curIndex){
				case 0:
					pass = subProcess(tMinX, tMinY, tMinZ, tMidX, tMidY, tMidZ, adjust, node->nodes[adjust], depth + 1, x, y);
					curIndex = nextProcess(tMidX, 4, tMidY, 2, tMidZ, 1);
					break;
				case 1:
					pass = subProcess(tMinX, tMinY, tMidZ, tMidX, tMidY, tMaxZ, adjust, node->nodes[1^adjust], depth + 1, x, y);
					curIndex = nextProcess(tMidX, 5, tMidY, 3, tMaxZ, 8);
					break;
				case 2:
					pass = subProcess(tMinX, tMidY, tMinZ, tMidX, tMaxY, tMidZ, adjust, node->nodes[2^adjust], depth + 1, x, y);
					curIndex = nextProcess(tMidX, 6, tMaxY, 8, tMidZ, 3);
					break;
				case 3:
					pass = subProcess(tMinX, tMidY, tMidZ, tMidX, tMaxY, tMaxZ, adjust, node->nodes[3^adjust], depth + 1, x, y);
					curIndex = nextProcess(tMidX, 7, tMaxY, 8, tMaxZ, 8);
					break;
				case 4:
					pass = subProcess(tMidX, tMinY, tMinZ, tMaxX, tMidY, tMidZ, adjust, node->nodes[4^adjust], depth + 1, x, y);
					curIndex = nextProcess(tMaxX, 8, tMidY, 6, tMidZ, 5);
					break;
				case 5:
					pass = subProcess(tMidX, tMinY, tMidZ, tMaxX, tMidY, tMaxZ, adjust, node->nodes[5^adjust], depth + 1, x, y);
					curIndex = nextProcess(tMaxX, 8, tMidY, 7, tMaxZ, 8);
					break;
				case 6:
					pass = subProcess(tMidX, tMidY, tMinZ, tMaxX, tMaxY, tMidZ, adjust, node->nodes[6^adjust], depth + 1, x, y);
					curIndex = nextProcess(tMaxX, 8, tMaxY, 8, tMidZ, 7);
					break;
				case 7:
					pass = subProcess(tMidX, tMidY, tMidZ, tMaxX, tMaxY, tMaxZ, adjust, node->nodes[7^adjust], depth + 1, x, y);
					curIndex = 8;
					break;
			}
			if(pass){
				return true;
			}
		}
	}
	return false;
}

void rayProcess(vec3 orig, vec3 dir, int x, int y){
	unsigned char adjust = 0;
	if(dir.x < 0){
		orig.x = ((tree->minX + ((tree->maxX - tree->minX) / 2.0)) * 2.0) - orig.x;
		dir.x = -dir.x;
		adjust |= 4;
	}
	if(dir.y < 0){
		orig.y = ((tree->minY + ((tree->maxY - tree->minY) / 2.0)) * 2.0) - orig.y;
		dir.y = -dir.y;
		adjust |= 2;
	}
	if(dir.z < 0){
		orig.z = ((tree->minZ + ((tree->maxZ - tree->minZ) / 2.0)) * 2.0) - orig.z;
		dir.z = -dir.z;
		adjust |= 1;
	}
	if(dir.x == 0){
		dir.x = FLT_MIN;
	}
	if(dir.y == 0){
		dir.y = FLT_MIN;
	}
	if(dir.z == 0){
		dir.z = FLT_MIN;
	}
	double tMinX = (tree->minX - orig.x) / dir.x;
	double tMinY = (tree->minY - orig.y) / dir.y;
	double tMinZ = (tree->minZ - orig.z) / dir.z;
	double tMaxX = (tree->maxX - orig.x) / dir.x;
	double tMaxY = (tree->maxY - orig.y) / dir.y;
	double tMaxZ = (tree->maxZ - orig.z) / dir.z;
	if(max(max(tMinX, tMinY), tMinZ) < min(min(tMaxX, tMaxY), tMaxZ)){
		subProcess(tMinX, tMinY, tMinZ, tMaxX, tMaxY, tMaxZ, adjust, tree->root, 1, x, y);
	}
}

void displayFunc(){
	glClear(GL_COLOR_BUFFER_BIT);

	vec3 origin, dir;
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

			rayProcess(origin, dir, x, y);
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

void specialFunc(int key, int x, int y){
	switch(key){
		case GLUT_KEY_UP:
			maxDepth++;
			break;
		case GLUT_KEY_DOWN:
			if(maxDepth > 1){
				maxDepth--;
			}
			break;
	}
	glutPostRedisplay();
}

int main(int argc, char** argv){
	tree = new octree();
	tree->maxX = 2;
	tree->minX = 0.5;
	tree->maxY = 2;
	tree->minY = 0.5;
	tree->maxZ = 2;
	tree->minZ = 0.5;
	tree->root = new octNode();

	octNode* node = tree->root;
	for(int i = 2; i < 7; i++){
		node->nodes[i] = new octNode();
		node->nodes[i]->term = true;
		node->nodes[i]->rgb[0] = rand() / (float) RAND_MAX;
		node->nodes[i]->rgb[1] = rand() / (float) RAND_MAX;
		node->nodes[i]->rgb[2] = rand() / (float) RAND_MAX;
	}



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
	glutSpecialFunc(specialFunc);

	glutMainLoop();
}
