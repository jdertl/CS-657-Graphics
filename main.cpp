#ifdef __APPLE__
#include <GLUT/glut.h>
#elif _WIN32
#include <windows.h>
  #include <GL/glut.h>
#elif __unix__
#include <GL/glut.h>
#endif
#include <float.h>
#include <iostream>
#include <map>
#include <math.h>
#include <stdio.h>
#include <list>
#include <iterator>

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
    int r, g, b;
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

struct cloud{
    int points;
    vec3 *vertices;
};

vec3 viewpoint = {0.0, 1.0, 5.0};
vec3 vp_u = {-1.0, 0.0, 0.0};
vec3 vp_v = {0.0, 1.0, 0.0};
vec3 vp_w = {0.0, 0.0, -1.0};
float vp_d = 1.0;

int angle = 180;
int height = 600;
int width = 600;

int maxRenderDepth = 1;
int maxDepth = 50;

int presetBinLevel = 5;

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

bool displayOriginPlatform = true;


void setPixel(int x, int y, float r, float g, float b){
    float* pixel = &displayBuffer[(y * width * 3) + (x * 3)];
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
}

bool finalProcess(octNode* node, int x, int y){
    setPixel(x, y, node->rgb[0], node->rgb[1], node->rgb[2]);
    return true;
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
        if(depth == maxRenderDepth || node->term){
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
            setPixel(x, y, 0.2, 0.2, 0.2);

            vec3 cutoff = origin + (100 * dir);
            if(displayOriginPlatform){
                float u = origin.y / (origin.y - cutoff.y);
                if(u >= 0){
                    float hitX = origin.x - ((origin.x - cutoff.x) * u);
                    float hitZ = origin.z - ((origin.z - cutoff.z) * u);
                    if(hitX >= -1 && hitX <= 1 && hitZ >= -1 && hitZ <= 1){
                        setPixel(x, y, 0.02, 0.02, 0.02);
                    }
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
        case 'z':
        case 'Z':
            displayOriginPlatform = !displayOriginPlatform;
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
            if(maxRenderDepth < maxDepth){
                maxRenderDepth++;
            }
            break;
        case GLUT_KEY_DOWN:
            if(maxRenderDepth > 1){
                maxRenderDepth--;
            }
            break;
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
            maxRenderDepth = 1;
    }
    cout << maxRenderDepth << endl;
    glutPostRedisplay();
}


bool comparePointMinMax(vec3 point,float minX,float maxX,float minY,float maxY,float minZ,float maxZ){
    if (point.x >= minX){
        if(point.x <= maxX){
            if (point.y >= minY){
                if(point.y <= maxY){
                    if (point.z >= minZ){
                        if(point.z <= maxZ){
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}


void BuildNode(octNode *node, list<vec3> pointList, float minX,float maxX,float minY,float maxY,float minZ,float maxZ, int level){
    //if presetBinLevel matches then we found the max depth of the tree
    //this node will be a terminal node
    //set color according to whatever points are left in the list
    node->nodes[0] = nullptr;
    node->nodes[1] = nullptr;
    node->nodes[2] = nullptr;
    node->nodes[3] = nullptr;
    node->nodes[4] = nullptr;
    node->nodes[5] = nullptr;
    node->nodes[6] = nullptr;
    node->nodes[7] = nullptr;

    if(level == presetBinLevel){
        float r = 0;
        float g = 0;
        float b = 0;
        float count = 0;
        std::list<vec3>::iterator it;

        it = pointList.begin();

        while(it != pointList.end()){
            r += it->r;
            g += it->g;
            b += it->b;
            count++;
            it++;
        }


        node->term = true;
        if(count > 0) {
            node->rgb[0] = (float)((r / count) / 255);
            node->rgb[1] = (float)((g / count) / 255);
            node->rgb[2] = (float)((b / count) / 255);
        }
        else{
            node->rgb[0] = 0;
            node->rgb[1] = 0;
            node->rgb[2] = 0;
        }
    }
        //we can still go deeper and subdivide the current block into smaller blocks
        //using pretty much the same steps as for the root
    else {

        float midX, midY, midZ;
        float r = 0;
        float g = 0;
        float b = 0;
        int j = 0;
        midX = (float) (minX + maxX) / 2;
        midY = (float) (minY + maxY) / 2;
        midZ = (float) (minZ + maxZ) / 2;

        std::list<vec3> lowerFrontLeft;
        std::list<vec3> lowerFrontRight;
        std::list<vec3> upperFrontLeft;
        std::list<vec3> upperFrontRight;
        std::list<vec3> lowerBackLeft;
        std::list<vec3> lowerBackRight;
        std::list<vec3> upperBackLeft;
        std::list<vec3> upperBackRight;

        std::list<vec3>::iterator it;

        it = pointList.begin();

            node->term = false;
            while (it != pointList.end()) {
                if (comparePointMinMax(*(it), minX, midX, minY, midY, minZ, midZ)) {
                    //in lowerFrontLeft
                    lowerFrontLeft.push_back(*(it));
                } else if (comparePointMinMax(*(it), midX, maxX, minY, midY, minZ, midZ)) {
                    //in lowerFrontRight
                    lowerFrontRight.push_back(*(it));
                } else if (comparePointMinMax(*(it), minX, midX, midY, maxY, minZ, midZ)) {
                    //in upperFrontLeft
                    upperFrontLeft.push_back(*(it));
                } else if (comparePointMinMax(*(it), midX, maxX, midY, maxY, minZ, midZ)) {
                    //in upperFrontRight
                    upperFrontRight.push_back(*(it));
                } else if (comparePointMinMax(*(it), minX, midX, minY, midY, midZ, maxZ)) {
                    //in lowerBackLeft
                    lowerBackLeft.push_back(*(it));
                } else if (comparePointMinMax(*(it), midX, maxX, minY, midY, midZ, maxZ)) {
                    //in lowerBackRight
                    lowerBackRight.push_back(*(it));
                } else if (comparePointMinMax(*(it), minX, midX, midY, maxY, midZ, maxZ)) {
                    //in upperBackLeft
                    upperBackLeft.push_back(*(it));
                } else if (comparePointMinMax(*(it), midX, maxX, midY, maxY, midZ, maxZ)) {
                    //in upperBackRight
                    upperBackRight.push_back(*(it));
                }
                it++;
                j++;
            }

            int count = 0;

            if (lowerFrontLeft.size() > 0) {

                node->nodes[0] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[0], lowerFrontLeft, minX, midX, minY, midY, minZ, midZ, level + 1);
                r += node->nodes[0]->rgb[0];
                g += node->nodes[0]->rgb[1];
                b += node->nodes[0]->rgb[2];
                count++;
                node->term = false;

            }
            if (lowerFrontRight.size() > 0) {

                node->nodes[1] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[1], lowerFrontRight, midX, maxX, minY, midY, minZ, midZ, level + 1);
                r += node->nodes[1]->rgb[0];
                g += node->nodes[1]->rgb[1];
                b += node->nodes[1]->rgb[2];
                count++;
                node->term = false;

            }
            if (upperFrontLeft.size() > 0) {

                node->nodes[2] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[2], upperFrontLeft, minX, midX, midY, maxY, minZ, midZ, level + 1);
                r += node->nodes[2]->rgb[0];
                g += node->nodes[2]->rgb[1];
                b += node->nodes[2]->rgb[2];
                count++;
                node->term = false;

            }
            if (upperFrontRight.size() > 0) {

                node->nodes[3] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[3], upperFrontRight, midX, maxX, midY, maxY, minZ, midZ, level + 1);
                r += node->nodes[3]->rgb[0];
                g += node->nodes[3]->rgb[1];
                b += node->nodes[3]->rgb[2];
                count++;
                node->term = false;

            }
            if (lowerBackLeft.size() > 0) {

                node->nodes[4] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[4], lowerBackLeft, minX, midX, minY, midY, midZ, maxZ, level + 1);
                r += node->nodes[4]->rgb[0];
                g += node->nodes[4]->rgb[1];
                b += node->nodes[4]->rgb[2];
                count++;
                node->term = false;

            }
            if (lowerBackRight.size() > 0) {

                node->nodes[5] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[5], lowerBackRight, midX, maxX, minY, midY, midZ, maxZ, level + 1);
                r += node->nodes[5]->rgb[0];
                g += node->nodes[5]->rgb[1];
                b += node->nodes[5]->rgb[2];
                count++;
                node->term = false;

            }
            if (upperBackLeft.size() > 0) {

                node->nodes[6] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[6], upperBackLeft, minX, midX, midY, maxY, midZ, maxZ, level + 1);
                r += node->nodes[6]->rgb[0];
                g += node->nodes[6]->rgb[1];
                b += node->nodes[6]->rgb[2];
                count++;
                node->term = false;

            }
            if (upperBackRight.size() > 0) {

                node->nodes[7] = (octNode *) malloc(sizeof(octNode));
                BuildNode(node->nodes[7], upperBackRight, midX, maxX, midY, maxY, midZ, maxZ, level + 1);
                r += node->nodes[7]->rgb[0];
                g += node->nodes[7]->rgb[1];
                b += node->nodes[7]->rgb[2];
                count++;
                node->term = false;

            }


            //assign color according to the average of the child nodes
            node->rgb[0] = (float) (r / count);
            node->rgb[1] = (float) (g / count);
            node->rgb[2] = (float) (b / count);
    }
}



//create 8 lists one for each node which will go 10 levels deep
//the lists will be bound by the respective min max values
// that way we don't have to search through all of the cloud structure as we try to find the points
octree* createTree(octree *tree, cloud *pointCloud){

    float midX, midY, midZ;
    float r = 0;
    float g = 0;
    float b = 0;

    midX = (float) (tree->minX + tree->maxX) / 2;
    midY = (float) (tree->minY + tree->maxY) / 2;
    midZ = (float) (tree->minZ + tree->maxZ) / 2;

    std::list<vec3> lowerFrontLeft;
    std::list<vec3> lowerFrontRight;
    std::list<vec3> upperFrontLeft;
    std::list<vec3> upperFrontRight;
    std::list<vec3> lowerBackLeft;
    std::list<vec3> lowerBackRight;
    std::list<vec3> upperBackLeft;
    std::list<vec3> upperBackRight;

    //create lists that contain the points that are pertinent to particular sub block
    for( int n = 0; n < pointCloud->points; n++){
        if ( comparePointMinMax(pointCloud->vertices[n],tree->minX, midX, tree->minY, midY, tree->minZ, midZ)){
            //in lowerFrontLeft
            lowerFrontLeft.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],midX, tree->maxX, tree->minY, midY, tree->minZ, midZ)){
            //in lowerFrontRight
            lowerFrontRight.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],tree->minX, midX, midY, tree->maxY, tree->minZ, midZ)){
            //in upperFrontLeft
            upperFrontLeft.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],midX, tree->maxX, midY, tree->maxY, tree->minZ, midZ)){
            //in upperFrontRight
            upperFrontRight.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],tree->minX, midX, tree->minY, midY, midZ, tree->maxZ)){
            //in lowerBackLeft
            lowerBackLeft.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],midX, tree->maxX, tree->minY, midY, midZ, tree->maxZ)){
            //in lowerBackRight
            lowerBackRight.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],tree->minX, midX, midY, tree->maxY, midZ, tree->maxZ)){
            //in upperBackLeft
            upperBackLeft.push_back(pointCloud->vertices[n]);
        }
        else if ( comparePointMinMax(pointCloud->vertices[n],midX, tree->maxX, midY, tree->maxY, midZ, tree->maxZ)){
            //in upperBackRight
            upperBackRight.push_back(pointCloud->vertices[n]);
        }
    }
    tree->root->term = false;

    tree->root->nodes[0] = nullptr;
    tree->root->nodes[1] = nullptr;
    tree->root->nodes[2] = nullptr;
    tree->root->nodes[3] = nullptr;
    tree->root->nodes[4] = nullptr;
    tree->root->nodes[5] = nullptr;
    tree->root->nodes[6] = nullptr;
    tree->root->nodes[7] = nullptr;


    ////build the nodes using the smaller lists of points and the respective min max values


    ////find color for root

    int count = 0;

    if(lowerFrontLeft.size() > 0){
        tree->root->nodes[0] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[0], lowerFrontLeft, tree->minX, midX, tree->minY, midY, tree->minZ, midZ, 0);
        r += tree->root->nodes[0]->rgb[0];
        g += tree->root->nodes[0]->rgb[1];
        b += tree->root->nodes[0]->rgb[2];
        count++;
    }
    if(lowerFrontRight.size() > 0){
        tree->root->nodes[1] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[1], lowerFrontRight,midX, tree->maxX, tree->minY, midY, tree->minZ, midZ, 0);
        r += tree->root->nodes[1]->rgb[0];
        g += tree->root->nodes[1]->rgb[1];
        b += tree->root->nodes[1]->rgb[2];
        count++;
    }
    if(upperFrontLeft.size() > 0){
        tree->root->nodes[2] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[2], upperFrontLeft, tree->minX, midX, midY, tree->maxY, tree->minZ, midZ, 0);
        r += tree->root->nodes[2]->rgb[0];
        g += tree->root->nodes[2]->rgb[1];
        b += tree->root->nodes[2]->rgb[2];
        count++;
    }
    if(upperFrontRight.size() > 0){
        tree->root->nodes[3] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[3], upperFrontRight,midX, tree->maxX, midY, tree->maxY, tree->minZ, midZ, 0);
        r += tree->root->nodes[3]->rgb[0];
        g += tree->root->nodes[3]->rgb[1];
        b += tree->root->nodes[3]->rgb[2];
        count++;
    }
    if(lowerBackLeft.size() > 0){
        tree->root->nodes[4] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[4], lowerBackLeft,  tree->minX, midX, tree->minY, midY, midZ, tree->maxZ, 0);
        r += tree->root->nodes[4]->rgb[0];
        g += tree->root->nodes[4]->rgb[1];
        b += tree->root->nodes[4]->rgb[2];
        count++;
    }
    if(lowerBackRight.size() > 0){
        tree->root->nodes[5] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[5], lowerBackRight, midX, tree->maxX, tree->minY, midY, midZ, tree->maxZ, 0);
        r += tree->root->nodes[5]->rgb[0];
        g += tree->root->nodes[5]->rgb[1];
        b += tree->root->nodes[5]->rgb[2];
        count++;
    }
    if(upperBackLeft.size() > 0){
        tree->root->nodes[6] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[6], upperBackLeft,  tree->minX, midX, midY, tree->maxY, midZ, tree->maxZ, 0);
        r += tree->root->nodes[6]->rgb[0];
        g += tree->root->nodes[6]->rgb[1];
        b += tree->root->nodes[6]->rgb[2];
        count++;
    }
    if(upperBackRight.size() > 0){
        tree->root->nodes[7] = (octNode*)malloc(sizeof(octNode));
        BuildNode(tree->root->nodes[7], upperBackRight, midX, tree->maxX, midY, tree->maxY, midZ, tree->maxZ, 0);
        r += tree->root->nodes[7]->rgb[0];
        g += tree->root->nodes[7]->rgb[1];
        b += tree->root->nodes[7]->rgb[2];
        count++;
    }


    tree->root->rgb[0] = (float)(r / count);
    tree->root->rgb[1] = (float)(g / count);
    tree->root->rgb[2] = (float)(b / count);

    return tree;
}



octree* readCloud(){
    int numPoints;
    float minX,minY,minZ,maxX,maxY,maxZ;
    float x,y,z;
    int r, g, b;
    char line[256];
    FILE *fin;
    cloud *pointCloud;
    octree *tree;

    if ((fin=fopen("bunny.ptx", "r"))==NULL){
        printf("read error...\n");
        exit(0);
    };

    fscanf(fin, "%d\n", &numPoints);
    fscanf(fin, "%f %f %f %f %f %f\n", &minX, &minY, &minZ, &maxX, &maxY, &maxZ);
    tree = (octree*)malloc(sizeof(octree));

    tree->minX = minX;
    tree->minY = minY;
    tree->minZ = minZ;
    tree->maxX = maxX;
    tree->maxY = maxY;
    tree->maxZ = maxZ;

    tree->root = (octNode*)malloc(sizeof(octNode));

    pointCloud = (cloud*)malloc(sizeof(cloud));
    pointCloud->points = numPoints;
    pointCloud->vertices = (vec3*)malloc(sizeof(vec3) * numPoints);

    //load all points in file onto cloud structure for easier traversal
    for( int n = 0; n < numPoints; n++) {
        fscanf(fin, "%f %f %f %d %d %d\n", &x, &y, &z,&r, &g, &b);

        pointCloud->vertices[n].x = x;
        pointCloud->vertices[n].y = y;
        pointCloud->vertices[n].z = z;
        pointCloud->vertices[n].r = r;
        pointCloud->vertices[n].g = g;
        pointCloud->vertices[n].b = b;
    }

    fclose(fin);

    tree = createTree(tree, pointCloud);
    return tree;
}




int main(int argc, char** argv){
    tree = new octree();

    octree* bunny = readCloud();
    tree->root = bunny->root;
    tree->minX = bunny->minX;
    tree->minY = bunny->minY;
    tree->minZ = bunny->minZ;
    tree->maxX = bunny->maxX;
    tree->maxY = bunny->maxY;
    tree->maxZ = bunny->maxZ;


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
    return 0;
}
