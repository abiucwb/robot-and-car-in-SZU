#include "Angel.h"
#include "TriMesh.h"
#include "mesh.h"
#include "Mesh_Painter.h"
#include <string>
#include <algorithm>
#include <assert.h>
#include "stb_image.h"
// #pragma comment(lib, "glew32.lib")
// #pragma comment(lib, "FreeImage.lib")

#include <iostream>
using namespace std;

//------------------------------------------------------------------------参数------------------------------------------------------------------------
typedef Angel::vec4 point4;
typedef Angel::vec4 color4;
// 存储要贴纹理的物体
std::vector<My_Mesh*>	my_meshs;
// 存储纹理图片
Mesh_Painter*			mp_;
//
GLuint program;
GLuint vao[2];
// 模视变换
GLuint ModelViewMatrixID;
mat4 ModelViewMatrix;
// 阴影
mat4 shadow;
GLuint shadowMatrixID;
// 透视参数
float fov = 100.0;
float aspect = 1.0;
float znear = 0.1;
float zfar = 100.0;
// 控制视角的角度初始值
float rad = 50.0;
float upAngle = 30;
float rotAngle = 0;
// 光源
vec3 lightPos(5, 10, 5);
GLuint lightPosID;
// 控制机器人行走的参数
// 切换行走姿势D
int runGesture = 1;
// 移动的坐标
float runZ = 0, runY = 0, runX = 0;	
//机器人的身体是否跟着镜头一起转动
int turnTogeter = 0;	 
// 切换第一视角行走模式
int t2f = 0;
// 控制机器人每一步的步长	
int stepSize = 2;	
// 绘制机器人时的位置偏移值
float increase = 1.0;	
// 用于传入片元着色器，区分光影
GLuint flag;
//空闲/分离模式
int mode=0;
//------------------------------------------------------------------------144------------------------------------------------------------------------

//------------------------------------------------------------------------相机------------------------------------------------------------------------
struct Camera
{
	vec4 eye=vec4(0.0, 0.0, 1.0, 1.0);
	vec4 at = vec4(0, 0, 0, 1);
	vec4 up = vec4(0, 1, 0, 0);
	float f=50;
	bool ortho = false;

	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projMatrix;

	// 相机观察函数
	mat4 getViewMatrix()
	{
		return LookAt(eye, at, up);
	}
	mat4 getProjectionMatrix()
	{
		mat4 projMatrix = mat4(1.0);
		if (ortho)
			projMatrix = Ortho(-f, f, -f, f, znear, zfar);
		else
			projMatrix = Perspective(fov, aspect, znear, zfar);
		return projMatrix;
	}
};
Camera camera;
//------------------------------------------------------------------------144------------------------------------------------------------------------

//------------------------------------------------------------------------机器人------------------------------------------------------------------------
const int NumVertices = 36; // 6*2*3 6个面，每个面包含2个三角面片，每个三角面片包含3个顶点 

point4 points[NumVertices];	// 点坐标
color4 colors[NumVertices];	// 点颜色
point4 normal[NumVertices];	// 法向量

point4 vertices[8] = {
	point4(-0.5, -0.5, 0.5, 1.0),
	point4(-0.5, 0.5, 0.5, 1.0),
	point4(0.5, 0.5, 0.5, 1.0),
	point4(0.5, -0.5, 0.5, 1.0),
	point4(-0.5, -0.5, -0.5, 1.0),
	point4(-0.5, 0.5, -0.5, 1.0),
	point4(0.5, 0.5, -0.5, 1.0),
	point4(0.5, -0.5, -0.5, 1.0)
};

// 颜色
color4 vertex_colors[9] = {
	color4(0.0, 0.0, 0.0, 1.0),  // 0black
	color4(1.0, 0.0, 0.0, 1.0),  // 1red
	color4(1.0, 1.0, 0.0, 1.0),  // 2yellow
	color4(0.0, 1.0, 0.0, 1.0),  // 3green
	color4(0.0, 0.0, 0.65, 1.0),  // 4blue(modified)
	color4(1.0, 0.0, 1.0, 1.0),  // 5magenta
	color4(1.0, 1.0, 1.0, 1.0),  // 6white
	color4(0.0, 0.5, 0.5, 1.0),  // 7cyan(dark)
	color4(0.6, 0.2, 0.2, 1.0)   // 8face
};

// 给机器人的各个部位设置颜色
point4 color_torso = vertex_colors[7];
point4 color_head = vertex_colors[8];
point4 color_upper_arm = vertex_colors[7];
point4 color_lower_arm = vertex_colors[8];
point4 color_upper_leg = vertex_colors[4];
point4 color_lower_leg = vertex_colors[8];
// point4 color_torso = vertex_colors[0];
// point4 color_head = vertex_colors[0];
// point4 color_upper_arm = vertex_colors[0];
// point4 color_lower_arm = vertex_colors[0];
// point4 color_upper_leg = vertex_colors[0];
// point4 color_lower_leg = vertex_colors[0];

// 定义机器人各个部位的宽和高
#define HEAD_HEIGHT 3.0
#define HEAD_WIDTH 3.0
#define TORSO_HEIGHT 10.5
#define TORSO_WIDTH 6.5
#define UPPER_ARM_HEIGHT 6.0
#define LOWER_ARM_HEIGHT 5.0
#define UPPER_ARM_WIDTH  1.5
#define LOWER_ARM_WIDTH  1.2
#define UPPER_LEG_HEIGHT 8.0
#define LOWER_LEG_HEIGHT 6.0
#define UPPER_LEG_WIDTH  2.2
#define LOWER_LEG_WIDTH  1.5

// 菜单选项，各部位可单独运动
enum {
	Torso,
	Head,
	Eyes,
	RightUpperArm,
	RightLowerArm,
	LeftUpperArm,
	LeftLowerArm,
	RightUpperLeg,
	RightLowerLeg,
	LeftUpperLeg,
	LeftLowerLeg,
	NumJointAngles,
	Quit
};

// 各部位旋转角度
GLfloat theta[NumJointAngles] = {
	0.0,    // Torso
	0.0,    // Head
	0.0,    // RightUpperArm
	0.0,    // RightLowerArm
	0.0,    // LeftUpperArm
	0.0,    // LeftLowerArm
	0.0,    // RightUpperLeg
	0.0,    // RightLowerLeg
	0.0,    // LeftUpperLeg
	0.0     // LeftLowerLeg
};

GLint angle = Eyes;

//------------------------------------------------------------------------144------------------------------------------------------------------------
class MatrixStack {
	int    _index;
	int    _size;
	mat4*  _matrices;

public:
	MatrixStack(int numMatrices = 100) :_index(0), _size(numMatrices)
	{
		_matrices = new mat4[numMatrices];
	}
	~MatrixStack()
	{
		delete[]_matrices;
	}
	void push(const mat4& m) {
		assert(_index + 1 < _size);
		_matrices[_index++] = m;
	}
	mat4& pop(void) {
		assert(_index - 1 >= 0);
		_index--;
		return _matrices[_index];
	}
};

MatrixStack  mvstack;
mat4         model_view;
GLuint       ModelView, Projection;
GLuint       draw_color;
//------------------------------------------------------------------------144------------------------------------------------------------------------
int Index = 0;
void quad(int a, int b, int c, int d)
{
	vec3 normalab = vec3(vertices[a].x - vertices[b].x, vertices[a].y - vertices[b].y, vertices[a].z - vertices[b].z);
	vec3 normalbc = vec3(vertices[b].x - vertices[c].x, vertices[b].y - vertices[c].y, vertices[b].z - vertices[c].z);
	vec3 normalcd = vec3(vertices[c].x - vertices[d].x, vertices[c].y - vertices[d].y, vertices[c].z - vertices[d].z);
	vec3 normalabc = cross(normalab, normalbc);
	colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; normal[Index] = normalabc; Index++;
	colors[Index] = vertex_colors[a]; points[Index] = vertices[b]; normal[Index] = normalabc; Index++;
	colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; normal[Index] = normalabc; Index++;
	colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; normal[Index] = normalabc; Index++;
	colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; normal[Index] = normalabc; Index++;
	colors[Index] = vertex_colors[a]; points[Index] = vertices[d]; normal[Index] = normalabc; Index++;
}

// 生成单位立方体的六个表面
void colorcube(void)
{
	quad(1, 0, 3, 2);
	quad(2, 3, 7, 6);
	quad(3, 0, 4, 7);
	quad(6, 5, 1, 2);
	quad(4, 5, 6, 7);
	quad(5, 4, 0, 1);
}
//------------------------------------------------------------------------144------------------------------------------------------------------------
// 为机器人添加阴影的函数
void add_robot_shadow()
{
	// 将阴影矩阵传入着色器
	glUniformMatrix4fv(shadowMatrixID, 1, GL_TRUE, &shadow[0][0]);
	// 设置颜色
	glUniform4fv(draw_color, 1, vertex_colors[0]);
	// 标记其为阴影
	glUniform1i(flag, 1);
	// 绘制
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
}

// 初始化阴影矩阵
void init_shadowMatrix()
{
	mat4 shadowMatrix = mat4(1.0);
	glUniformMatrix4fv(shadowMatrixID, 1, GL_TRUE, &shadowMatrix[0][0]);
	glUniformMatrix4fv(ModelViewMatrixID, 1, GL_TRUE, &ModelViewMatrix[0][0]);
}

void torso()
{
	// 初始化阴影矩阵
	init_shadowMatrix();
	// 保存父节点矩阵
	mvstack.push(model_view);
	// 本节点局部变换矩阵
	mat4 instance = (
		Translate(0.0, 0.5 * TORSO_HEIGHT, 0.0) *
		Scale(TORSO_WIDTH, TORSO_HEIGHT, TORSO_WIDTH / 2));
	// 父节点矩阵 * 本节点局部变换矩阵
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	// 绑定绘制颜色
	glUniform4fv(draw_color, 1, color_torso);
	// 标记其不为阴影
	glUniform1i(flag, 0);	
	// 绘制
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	// 为此部分绘制阴影
	add_robot_shadow(); 
	model_view = mvstack.pop();// 恢复父节点矩阵（下同）
}

void head()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * HEAD_HEIGHT, 0.0) *
		Scale(HEAD_WIDTH, HEAD_HEIGHT, HEAD_WIDTH));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_head);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}


void left_upper_arm()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0) *
		Scale(UPPER_ARM_WIDTH + 0.3, UPPER_ARM_HEIGHT, UPPER_ARM_WIDTH + 0.3));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_upper_arm);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}

void left_lower_arm()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0) *
		Scale(LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT, LOWER_ARM_WIDTH));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_lower_arm);	
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);	
	add_robot_shadow();	
	model_view = mvstack.pop();
}

void right_upper_arm()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0) *
		Scale(UPPER_ARM_WIDTH + 0.3, UPPER_ARM_HEIGHT, UPPER_ARM_WIDTH + 0.3));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_upper_arm);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}

void right_lower_arm()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0) *
		Scale(LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT, LOWER_ARM_WIDTH));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_lower_arm);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}

void left_upper_leg()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * UPPER_LEG_HEIGHT, 0.0) *
		Scale(UPPER_LEG_WIDTH + 0.3, UPPER_LEG_HEIGHT, UPPER_LEG_WIDTH + 0.3));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_upper_leg);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}

void left_lower_leg()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * LOWER_LEG_HEIGHT, 0.0) *
		Scale(LOWER_LEG_WIDTH, LOWER_LEG_HEIGHT, LOWER_LEG_WIDTH));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_lower_leg);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}

void right_upper_leg()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * UPPER_LEG_HEIGHT, 0.0) *
		Scale(UPPER_LEG_WIDTH + 0.3, UPPER_LEG_HEIGHT, UPPER_LEG_WIDTH + 0.3));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_upper_leg);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}

void right_lower_leg()
{
	init_shadowMatrix();
	mvstack.push(model_view);
	mat4 instance = (Translate(0.0, 0.5 * LOWER_LEG_HEIGHT, 0.0) *
		Scale(LOWER_LEG_WIDTH, LOWER_LEG_HEIGHT, LOWER_LEG_WIDTH));
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
	glUniform4fv(draw_color, 1, color_lower_leg);
	glUniform1i(flag, 0);
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	add_robot_shadow();
	model_view = mvstack.pop();
}
//------------------------------------------------------------------------144------------------------------------------------------------------------
//机器人层级连接
void show_robot()
{
	glUseProgram(program);
	glBindVertexArray(vao[0]);

	model_view = Translate(runZ * sin(rotAngle*DegreesToRadians) + runX, 0, runZ * cos(rotAngle*DegreesToRadians)) 
		* Translate(0.0, increase + UPPER_LEG_HEIGHT + LOWER_LEG_HEIGHT, 0.0)*RotateY(theta[Torso]);//躯干变换矩阵
	torso();//躯干绘制
	mvstack.push(model_view);//保存躯干变换矩阵
	model_view *= (Translate(0.0, TORSO_HEIGHT, 0.0) *RotateY(theta[Head]));
	head();//头部绘制
	model_view = mvstack.pop();//恢复躯干变换矩阵
	mvstack.push(model_view); //保存躯干变换矩阵

	//乘以上臂的变换矩阵(注意此处最后乘了RotateX，代表改变theta[LeftUpperArm]会改变上臂的旋转角度，以X轴为旋转轴)
	//左上臂绘制
	model_view *= (Translate(0.5*(TORSO_WIDTH + UPPER_ARM_WIDTH), 0.95 * TORSO_HEIGHT, 0.0) *RotateX(theta[LeftUpperArm])*RotateZ(180));
	left_upper_arm();
	//乘以下臂的变换矩阵(注意此处最后乘了RotateX，意义同上)
	//左下臂绘制
	model_view *= (Translate(0.0, UPPER_ARM_HEIGHT, 0.0) *RotateX(theta[LeftLowerArm]));
	left_lower_arm();
	model_view = mvstack.pop();//恢复躯干变换矩阵
	mvstack.push(model_view); //保存躯干变换矩阵
	//右上臂绘制
	model_view *= (Translate(-0.5*(TORSO_WIDTH + UPPER_ARM_WIDTH), 0.95 * TORSO_HEIGHT, 0.0) *RotateX(theta[RightUpperArm])*RotateZ(180));
	right_upper_arm();
	//右下臂绘制
	model_view *= (Translate(0.0, UPPER_ARM_HEIGHT, 0.0) *RotateX(theta[RightLowerArm]));
	right_lower_arm();
	model_view = mvstack.pop();//恢复躯干变换矩阵

	mvstack.push(model_view); //保存躯干变换矩阵
	model_view *= (Translate(TORSO_HEIGHT / 6, 0, 0.0) *RotateX(theta[LeftUpperLeg])*RotateZ(180));
	left_upper_leg();//左上腿绘制
	model_view *= (Translate(0.0, UPPER_LEG_HEIGHT, 0.0) *RotateX(theta[LeftLowerLeg]));
	left_lower_leg();//左下腿绘制
	model_view = mvstack.pop();//恢复躯干变换矩阵

	mvstack.push(model_view); //保存躯干变换矩阵
	model_view *= (Translate(-TORSO_HEIGHT / 6, 0, 0.0) *RotateX(theta[RightUpperLeg])*RotateZ(180));
	right_upper_leg();//右上腿绘制
	model_view *= (Translate(0.0, UPPER_LEG_HEIGHT, 0.0) *RotateX(theta[RightLowerLeg]));
	right_lower_leg();//右下腿绘制
	model_view = mvstack.pop();//恢复躯干变换矩阵
}

// 控制机器人姿势的函数
void robotChangeGesture() {
	if (runGesture == 1) {
		theta[LeftUpperArm] = 50;
		theta[LeftLowerArm] = 120;
		theta[RightUpperArm] = -50;
		theta[RightLowerArm] = 100;
		theta[RightUpperLeg] = 20;
		theta[RightLowerLeg] = 300;
		theta[LeftUpperLeg] = 350;
		theta[LeftLowerLeg] = 0.0;
		runGesture = 0;
	}
	else {
		theta[LeftUpperArm] = 350;
		theta[LeftLowerArm] = 105;
		theta[RightUpperArm] = 50;
		theta[RightLowerArm] = 110;
		theta[RightUpperLeg] = 350;
		theta[RightLowerLeg] = 0;
		theta[LeftUpperLeg] = 20;
		theta[LeftLowerLeg] = 300;
		runGesture = 1;
	}
}

void reset() {
	theta[LeftUpperArm] = 0;
	theta[LeftLowerArm] = 0;
	theta[RightUpperArm] = 0;
	theta[RightLowerArm] = 0;
	theta[RightUpperLeg] = 0;
	theta[RightLowerLeg] = 0;
	theta[LeftUpperLeg] = 0;
	theta[LeftLowerLeg] = 0;
}

//------------------------------------------------------------------------144------------------------------------------------------------------------
//适配窗口大小
void reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	GLfloat left = -10.0, right = 10.0;
	GLfloat bottom = -5.0, top = 15.0;
	GLfloat zNear = -10.0, zFar = 10.0;

	GLfloat aspect = GLfloat(width) / height;

	if (aspect > 1.0) {
		left *= aspect;
		right *= aspect;
	}
	else {
		bottom /= aspect;
		top /= aspect;
	}

	mat4 projection = Ortho(left, right, bottom, top, zNear, zFar);
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

	model_view = mat4(1.0);
}
//------------------------------------------------------------------------init------------------------------------------------------------------------
void init()
{
	colorcube();

	// 创建顶点数组对象
	glGenVertexArrays(1, &vao[0]);
	glBindVertexArray(vao[0]);

	// 创建顶点缓存对象
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors) + sizeof(normal),NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors),colors);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), sizeof(normal), normal);

	// 读取着色器
	program = InitShader("shaders/vshader_win.glsl", "shaders/fshader_win.glsl");

	ModelViewMatrixID = glGetUniformLocation(program, "modelViewMatrix");
	shadowMatrixID = glGetUniformLocation(program, "shadowMatrix");

	glUseProgram(program);

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
	BUFFER_OFFSET(0));

	GLuint vColor = glGetAttribLocation(program, "vColor");
	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
	BUFFER_OFFSET(sizeof(points)));

	ModelView = glGetUniformLocation(program, "ModelView");
	Projection = glGetUniformLocation(program, "Projection");
	draw_color = glGetUniformLocation(program, "draw_color");
	lightPosID = glGetUniformLocation(program, "lightPos");
	flag = glGetUniformLocation(program, "flag");

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
	BUFFER_OFFSET(sizeof(points) + sizeof(colors)));

	// OpenGL相应状态设置
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// 黑色背景
	glClearColor(0.0, 0.0, 0.0, 0.0);
	// // 白色背景
	// glClearColor(1.0, 1.0, 1.0, 0.0);

}

//------------------------------------------------------------------------display------------------------------------------------------------------------

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// eye 的xyz在镜头旋转时的变化
	float x = rad * cos(upAngle * DegreesToRadians) * sin(rotAngle * DegreesToRadians);
	float y = rad * sin(upAngle * DegreesToRadians) ;
	float z = rad * cos(upAngle * DegreesToRadians) * cos(rotAngle * DegreesToRadians);
	
	// at 的xyz参数
	float atx = 0;
	float aty = 22.5;
	float atz = 0;

	// 如果第一视角行走，eye需跟着机器人走，eye与at保持距离不变
	if (t2f == 1) {
		x += (runZ-37)*sin(rotAngle*DegreesToRadians);
		y += 10;
		z += (runZ-37)*cos(rotAngle*DegreesToRadians);

		atx += (runZ-37) *sin(rotAngle*DegreesToRadians);
		aty += 0;
		atz += (runZ-37)*cos(rotAngle*DegreesToRadians);
	}

	camera.eye=vec4(x, y, z, 1);
	camera.at=vec4(atx, aty, atz, 1);
	camera.up=vec4(0, 1, 0, 0);

	// 光源位置
	float lx = lightPos[0];
	float ly = lightPos[1];
	float lz = lightPos[2];

	//函数方程为 Ax+By+Cz+D=0 ,所以n表示阴影所需要投影的平面
	//生成阴影矩阵，生成阴影的投影和透视矩阵，传入着色器
	GLfloat n[] = { 0,1,0,-1 };
	shadow = mat4(
		vec4(-ly*n[1] - lz*n[2], lx*n[1], lx*n[2], lx*n[3]),
		vec4(ly*n[0], -lx*n[0] - lz*n[2], ly*n[2], ly*n[3]),
		vec4(lz*n[0], lz*n[1], -lx*n[0] - ly*n[1], lx*n[3]),
		vec4(0, 0, 0, -lx*n[0] - ly*n[1] - lz*n[2]));

	camera.modelMatrix = mat4(1.0);
	camera.viewMatrix = camera.getViewMatrix();
	camera.projMatrix = camera.getProjectionMatrix();
	glUniformMatrix4fv(Projection, 1, GL_TRUE, &camera.projMatrix[0][0]);
	glUniform3fv(lightPosID, 1, &lightPos[0]);	// 将光源信息传入顶点着色器

	ModelViewMatrix = camera.viewMatrix * camera.modelMatrix;

	mp_->setCameraMatrix(camera.modelMatrix, camera.viewMatrix, camera.projMatrix);
	mp_->draw_meshes();//画图形
	
	glUseProgram(program);
	show_robot();	// 绘制机器人

	glutSwapBuffers();
};

//------------------------------------------------------------------------鼠标------------------------------------------------------------------------

// 拖动鼠标控制光源位置的参数
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraDistance;
float cameraAngleX;
float cameraAngleY;

int change=0;
void mouse(int button, int state, int x, int y)
{
	//检测鼠标是否按下的状态，并记录下鼠标坐标
	mouseX = x;
	mouseY = y;
	//按下鼠标
	if (button == GLUT_LEFT_BUTTON)
	{
		//鼠标是否被按住
		if (state == GLUT_DOWN)
		{
			mouseLeftDown = true;
		}
		else if (state == GLUT_UP)
			mouseLeftDown = false;
	}
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			mouseRightDown = true;
		}
		else if (state == GLUT_UP)
			mouseRightDown = false;
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		theta[angle] += 20.0;
		//cout << theta[angle] << endl;
		if (theta[angle] > 360.0) { theta[angle] -= 360.0; }
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		theta[angle] -= 20.0;
		//cout << theta[angle] << endl;
		if (theta[angle] < 0.0) { theta[angle] += 360.0; }
	}
	glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
	//拖动调光源位置
	if (mouseLeftDown)
	{
		//算点击处和当前鼠标位置的距离
		cameraAngleY += (x - mouseX);
		cameraAngleX += (y - mouseY);
		if (y>mouseY)
		{
			lightPos += vec3(0, (mouseY - y) / 50, 0);

		}
		else if (y < mouseY)
		{
			lightPos += vec3(0, (mouseY - y) / 50, 0);

		}
		if (x>mouseX)
		{
			lightPos += vec3((x - mouseX) / 50, 0, 0);

		}
		else if (x<mouseX)
		{
			lightPos += vec3((x - mouseX) / 50, 0, 0);

		}
		mouseX = x;
		mouseY = y;
	}

	if (lightPos[1] <= 1)
		lightPos[1] = 1;

	glutPostRedisplay();
}
//------------------------------------------------------------------------144------------------------------------------------------------------------

void showpos(){
	cout << "x=" << camera.eye.x << " y=" << camera.eye.y << " z=" << camera.eye.z << endl;
	cout << "runx=" << runX << "runz=" << runZ <<endl;
}
void special_key(int key,int x,int y){
	switch(key){
		case GLUT_KEY_UP:
		rad -= 5;
		//显示相机位置
		showpos();
		break;
		case GLUT_KEY_DOWN:
		rad += 5;
		showpos();
		break;
		case GLUT_KEY_LEFT:
		fov -= 5;
		showpos();
		break;
		case GLUT_KEY_RIGHT:
		fov += 5;
		showpos();
		break;
	}
}

void keyboard(unsigned char key,int a,int b)
{
	switch (key)
	{
	//相机
	//切换视角（有问题）
	case 'o':
		camera.ortho = true;
		break;
	case 'p':
		camera.ortho = false;
	//不同角度
	case 'z':
		upAngle = 30.0;
		rotAngle = 50.0;
		break;
	case 'x':
		upAngle = 30.0;
		rotAngle = 140.0;
		break;
	case 'c':
		upAngle = 30.0;
		rotAngle = 230;
		break;
	case 'v':
		upAngle = 30.0;
		rotAngle = 320;
		break;
	//移动相机
	case 'w': 
		upAngle += 5;
		showpos();
		break;
	case 's': 
		upAngle -= 5;
		showpos();
		break;
	case 'd':
		rotAngle += 10;	// 镜头转
		if (turnTogeter == 1) {
			theta[Torso] += 10;	// 机器人随镜头一起转
		}
		showpos();
		break;
	case 'a':
		if (turnTogeter == 1) {
			theta[Torso] -= 10;
		}
		showpos();
		rotAngle -= 10;
		break;

	// 控制机器人移动
	case 'i':	// 前进
		robotChangeGesture();// 摆姿势
		runZ -= stepSize;
		theta[Torso] = (int)rotAngle % 360 + 180.0;
		showpos();
		break;
	case 'k':	// 后退
		robotChangeGesture();
		runZ += stepSize;
		if (t2f == 1) {
			theta[Torso] = (int)rotAngle % 360+180.0;
			showpos();
		}
		else {
			theta[Torso] = (int)rotAngle % 360;
			showpos();
		}
		break;
	case 'j':// 左移
		robotChangeGesture();	
		// 第一人称
		if (t2f == 1) {
			runZ -= stepSize;	// 走出一步，步长为stepSize
			theta[Torso] = (int)rotAngle % 360 + 180.0 + 45.0;// 机器人身体转一下角度
			rotAngle += 10;
			showpos();
		}
		// 第三人称
		else {
			runX -= stepSize;	
			theta[Torso] = 180.0 + 90.0;
			showpos();
		}
		break;
	case 'l':	// 右移
		robotChangeGesture();
		if (t2f == 1) {
			runZ -= stepSize;
			theta[Torso] = (int)rotAngle % 360 + 180.0 - 45.0;
			rotAngle -= 10;
			showpos();
		}
		else {
			runX += stepSize;
			theta[Torso] = 180.0 - 90.0;
			showpos();
		}
		break;

	// 控制机器人是否和镜头一起转
	case 'b':
		if (turnTogeter == 0) {
			turnTogeter = 1;
		}
		else if (turnTogeter == 1) {
			turnTogeter = 0;
		}
		break;

	// 切换机器人第一视角和第三视角
	case ' ':	
		if (t2f == 0) {
			t2f = 1;
			runX = 0;
			theta[Torso] = (int)rotAngle % 360 + 180.0;
		}
		else if (t2f == 1) {
			t2f = 0;
			rad = 50.0;
			upAngle = 30.0;
			rotAngle = 0;
			showpos();
		}
		break;
	// 机器人姿势重置
	case 't':
		theta[Torso] = 0;
		theta[Head] = 0;
		theta[Eyes] = 0;
		theta[LeftUpperArm] = 0;
		theta[LeftLowerArm] = 0;
		theta[RightUpperArm] = 0;
		theta[RightLowerArm] = 0;
		theta[RightUpperLeg] = 0;
		theta[RightLowerLeg] = 0;
		theta[LeftUpperLeg] = 0;
		theta[LeftLowerLeg] = 0;
		break;
	// 所有信息重置
	case 'r':
		// camera
		rad = 50.0;
		upAngle = 50.0;
		rotAngle = 50.0;

		// perspective
		fov = 100.0;
		aspect = 1.0;
		znear = 0.1;
		zfar = 100.0;
		theta[Torso] = 0.0;
		runZ = 0;
		runX = 0;
		lightPos = vec3(5, 10, 5);

		// 机器人姿势
		theta[Torso] = 0;
		theta[Head] = 0;
		theta[Eyes] = 0;
		theta[LeftUpperArm] = 0;
		theta[LeftLowerArm] = 0;
		theta[RightUpperArm] = 0;
		theta[RightLowerArm] = 0;
		theta[RightUpperLeg] = 0;
		theta[RightLowerLeg] = 0;
		theta[LeftUpperLeg] = 0;
		theta[LeftLowerLeg] = 0;
		// control
		turnTogeter = 0;
		t2f = 0;
		break;
	case 'f':
		my_meshs[2] ->set_theta(0, -90, 0);
		my_meshs[2] ->set_translate(-36, 6, -10.5);
		break;
	case 'F':
		my_meshs[2] ->set_theta(0, 0, 90);
		my_meshs[2] ->set_translate(-35, 4, -2);
		break;
	case 'm':
		mode++;
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

//----------------------------------------------------------------------------
//空闲回调
void idle(void)
{
	if(runX==-30 && runZ==-2){
		my_meshs[2] ->set_theta(0, -90, 0);
		my_meshs[2] ->set_translate(-36, 6, -10.5);
	}
	if(mode%2==0){
		Sleep(200);
		reset();
	}
	glutPostRedisplay();
}
// 窗口大小
struct Window
{
	int width = 1000;
	int height = 1000;
};
Window window;

void menu(int option)
{
	if (option == Quit) {
		exit(EXIT_SUCCESS);
	}
	angle = option;
}

int main( int argc, char **argv )
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(window.width, window.height);
	glutCreateWindow("2017044006_陈炜标_期末大作业");

	#ifdef WIN32
		glutInitContextVersion(3, 2);
		glutInitContextProfile(GLUT_CORE_PROFILE);
		glewExperimental = GL_TRUE;
		glewInit();
	#endif

	init();
	mp_ = new Mesh_Painter;

	My_Mesh* my_mesh_car = new My_Mesh;
	my_mesh_car->load_obj("texture/car.obj",100);
	my_mesh_car->set_texture_file("texture/car.bmp");	// 指定纹理图像文件
	my_mesh_car->set_translate(-20, 12, -20);	// 平移
	my_mesh_car->set_theta(0, 90, 0);
	mp_->add_mesh(my_mesh_car);
	my_meshs.push_back(my_mesh_car);
	add_robot_shadow();

	My_Mesh* my_mesh_tyre_yq = new My_Mesh;
	my_mesh_tyre_yq ->load_obj("texture/wheel.obj",24);
	my_mesh_tyre_yq ->set_texture_file("texture/tyre.bmp");	// 指定纹理图像文件
	my_mesh_tyre_yq ->set_translate(-5, 6, -10);	// 平移
	my_mesh_tyre_yq->set_theta(0, -50, 0);
	mp_->add_mesh(my_mesh_tyre_yq);
	my_meshs.push_back(my_mesh_tyre_yq);

	My_Mesh* my_mesh_tyre_yh = new My_Mesh;
	my_mesh_tyre_yh ->load_obj("texture/wheel.obj",24);
	my_mesh_tyre_yh ->set_texture_file("texture/tyre.bmp");	// 指定纹理图像文件
	my_mesh_tyre_yh ->set_translate(-35, 4, -2);	// 平移
	my_mesh_tyre_yh->set_theta(0, 0, 90);
	mp_->add_mesh(my_mesh_tyre_yh);
	my_meshs.push_back(my_mesh_tyre_yh);

	My_Mesh* my_mesh_tyre_zq = new My_Mesh;
	my_mesh_tyre_zq ->load_obj("texture/wheel.obj",24);
	my_mesh_tyre_zq ->set_texture_file("texture/tyre.bmp");	// 指定纹理图像文件
	my_mesh_tyre_zq ->set_translate(-4, 6, -29);	// 平移
	my_mesh_tyre_zq->set_theta(0, 130, 0);
	mp_->add_mesh(my_mesh_tyre_zq);
	my_meshs.push_back(my_mesh_tyre_zq);

	My_Mesh* my_mesh_tyre_zh = new My_Mesh;
	my_mesh_tyre_zh ->load_obj("texture/wheel.obj",24);
	my_mesh_tyre_zh ->set_texture_file("texture/tyre.bmp");	// 指定纹理图像文件
	my_mesh_tyre_zh ->set_translate(-36, 6, -28.5);	// 平移
	my_mesh_tyre_zh->set_theta(0, 90, 0);
	mp_->add_mesh(my_mesh_tyre_zh);
	my_meshs.push_back(my_mesh_tyre_zh);

	//墙壁
	My_Mesh* my_mesh_fw = new My_Mesh;
	my_mesh_fw->generate_wall_fb(4);
	my_mesh_fw->set_texture_file("texture/front.jpg");
	my_mesh_fw->set_translate(0, 0, 0);
	my_mesh_fw->set_theta(0, 0, 0);
	my_meshs.push_back(my_mesh_fw);
	mp_->add_mesh(my_mesh_fw);

	My_Mesh* my_mesh_lw = new My_Mesh;
	my_mesh_lw->generate_wall_lr(4);
	my_mesh_lw->set_texture_file("texture/left.jpg");
	my_mesh_lw->set_translate(0, 0, 0);
	my_mesh_lw->set_theta(0, 0, 0);
	my_meshs.push_back(my_mesh_lw);
	mp_->add_mesh(my_mesh_lw);

	My_Mesh* my_mesh_bw = new My_Mesh;
	my_mesh_bw->generate_wall_fb(4);
	my_mesh_bw->set_texture_file("texture/back.jpg");
	my_mesh_bw->set_translate(0, 0, 166.4);
	my_mesh_bw->set_theta(0, 0, 0);
	my_meshs.push_back(my_mesh_bw);
	mp_->add_mesh(my_mesh_bw);

	My_Mesh* my_mesh_rw = new My_Mesh;
	my_mesh_rw ->generate_wall_lr(4);
	my_mesh_rw ->set_texture_file("texture/right.jpg");
	my_mesh_rw ->set_translate(166.4, 0, 0);
	my_mesh_rw ->set_theta(0, 0, 0);
	my_meshs.push_back(my_mesh_rw );
	mp_->add_mesh(my_mesh_rw );

	//生成地面和天花板
	My_Mesh* my_mesh_fl = new My_Mesh;
	my_mesh_fl->generate_floor(4);	// 生成平面需要的点、面、向量
	my_mesh_fl->set_texture_file("texture/bottom.jpg");//指定纹理图像文件
	my_mesh_fl->set_translate(0, 0, 0);	// 平移
	my_mesh_fl->set_theta(0, 0, 0);	// 旋转轴
	my_meshs.push_back(my_mesh_fl);	
	mp_->add_mesh(my_mesh_fl);

	My_Mesh* my_mesh_top = new My_Mesh;
	my_mesh_top->generate_floor(4);
	my_mesh_top->set_texture_file("texture/top.jpg");
	my_mesh_top->set_translate(0, 166.4, 0);
	my_mesh_top->set_theta(0, 0, 0);
	my_meshs.push_back(my_mesh_top);
	mp_->add_mesh(my_mesh_top);

	std::string vshader, fshader;
	vshader = "shaders/v_texture.glsl";
	fshader = "shaders/f_texture.glsl";
	mp_->init_shaders(vshader.c_str(), fshader.c_str());
	mp_->update_vertex_buffer();
	mp_->update_texture();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special_key);
	glutMouseFunc(mouse);
	glutIdleFunc(idle);

	// 创建菜单
	glutCreateMenu(menu);
	glutAddMenuEntry("head", Head);
	glutAddMenuEntry("torso", Torso);
	glutAddMenuEntry("right_upper_arm", RightUpperArm);
	glutAddMenuEntry("right_lower_arm", RightLowerArm);
	glutAddMenuEntry("left_upper_arm", LeftUpperArm);
	glutAddMenuEntry("left_lower_arm", LeftLowerArm);
	glutAddMenuEntry("right_upper_leg", RightUpperLeg);
	glutAddMenuEntry("right_lower_leg", RightLowerLeg);
	glutAddMenuEntry("left_upper_leg", LeftUpperLeg);
	glutAddMenuEntry("left_lower_leg", LeftLowerLeg);
	glutAddMenuEntry("quit", Quit);
	glutAttachMenu(GLUT_MIDDLE_BUTTON);	// 指定弹出菜单键为鼠标中键

	glutMotionFunc(mouseMotion);

	glutMainLoop();

	for (unsigned int i = 0; i < my_meshs.size(); i++)
	{
		delete my_meshs[i];
	}
	delete mp_;

	return 0;
}
