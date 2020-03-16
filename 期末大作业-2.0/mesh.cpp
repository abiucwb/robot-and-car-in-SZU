#include "mesh.h"
#include <sstream>
#include <fstream>
#include <iosfwd>
#include <algorithm>
#include <math.h>
#include <array>
#include <vector>

#include <gl/GL.h>


#include <string>
using namespace std;

My_Mesh::My_Mesh()
{
	vTranslation[0] = Theta[0] = 0;
	vTranslation[1] = Theta[1] = 0;
	vTranslation[2] = Theta[2] = 0;
	Theta[0] = 45;
}

My_Mesh::~My_Mesh()
{
}

void My_Mesh::load_obj(std::string obj_File,double scale)
{
	// @TODO: 请在此添加代码实现对含有UV坐标的obj文件的读取
	// 将读取的数据存储为My_Mesh对象的属性值，可参考圆柱体的生成
	// fin打开文件读取文件信息
	if (obj_File.empty()) {
		return;
	}
	std::ifstream fin;
	fin.open(obj_File);
	if (!fin)
	{
		cout<<"文件有误"<<endl;
		return;
	}
	else
	{
		cout<<"文件打开成功!"<<endl;
		this->clear_data();
		this->m_center_ = point3f(0, 0, 0);
		float xmin=100,xmax=-100,ymin=100,ymax=-100,zmin=100,zmax=-100;
		string str;
		while(1){
			fin>>str;
			if(str=="faces") break;
			if(str=="v"){
				float x,y,z;
				fin>>x>>y>>z;
				if(x>xmax) xmax=x;
				else if(x<xmin) xmin=x;
				if(y>ymax) ymax=y;
				else if(y<ymin) ymin=y;
				if(z>zmax) zmax=z;
				else if(z<zmin) zmin=z;
				m_vertices_.push_back(x);
				m_vertices_.push_back(y);
				m_vertices_.push_back(z);
			}			
			if(str=="vn"){
				float x,y,z;
				fin>>x>>y>>z;
				m_normals_.push_back(x);
				m_normals_.push_back(y);
				m_normals_.push_back(z);

				float r;
				float g;
				float b;
				My_Mesh::normal_to_color(x, y, z, r, g, b);
				// 这里采用法线来生成颜色，学生可以自定义自己的颜色生成方式
				m_color_list_.push_back(r);
				m_color_list_.push_back(g);
				m_color_list_.push_back(b);
			}
			if(str=="vt"){
				float x,y,z;
				fin>>x>>y>>z;
				m_vt_list_.push_back(x);
				m_vt_list_.push_back(y);
				m_vt_list_.push_back(z);
			}
			if(str=="f"){
				char get;
				float x1,x2,x3,y1,y2,y3,z1,z2,z3;
				fin
				>>x1>>get>>x2>>get>>x3
				>>y1>>get>>y2>>get>>y3
				>>z1>>get>>z2>>get>>z3;
				//检验数据
				// cout
				// <<x1<<" "<<x2<<" "<<x3<<" "
				// <<y1<<" "<<y2<<" "<<y3<<" "
				// <<z1<<" "<<z2<<" "<<z3<<endl;
				m_faces_.push_back(x1);
				m_faces_.push_back(x2);
				m_faces_.push_back(x3);
				m_faces_.push_back(y1);
				m_faces_.push_back(y2);
				m_faces_.push_back(y3);
				m_faces_.push_back(z1);
				m_faces_.push_back(z2);
				m_faces_.push_back(z3);
			}
		}
		cout<<"xmin: "<<xmin<<" xmax: "<<xmax
			<<" ymin: "<<ymin<<" ymax: "<<ymax
			<<" zmin: "<<zmin<<" zmax: "<<zmax
			<<endl;
		this->m_min_box_ = point3f(xmin/scale, ymin/scale, zmin/scale);//最小值
		this->m_max_box_ = point3f(xmax/scale, ymax/scale, zmax/scale);//最大值
	}
};

void My_Mesh::normal_to_color(float nx, float ny, float nz, float& r, float& g, float& b)
{
	r = float(std::min(std::max(0.5 * (nx + 1.0), 0.0), 1.0));
	g = float(std::min(std::max(0.5 * (ny + 1.0), 0.0), 1.0));
	b = float(std::min(std::max(0.5 * (nz + 1.0), 0.0), 1.0));
};

const VtList&  My_Mesh::get_vts()
{
	return this->m_vt_list_;
};

void My_Mesh::clear_data()
{
	m_vertices_.clear();
	m_normals_.clear();
	m_faces_.clear();
	m_color_list_.clear();
	m_vt_list_.clear();
};

void My_Mesh::get_boundingbox(point3f& min_p, point3f& max_p) const
{
	min_p = this->m_min_box_;
	max_p = this->m_max_box_;
};

const STLVectorf&  My_Mesh::get_colors()
{
	return this->m_color_list_;
};

const VertexList& My_Mesh::get_vertices()
{
	return this->m_vertices_;
};

const NormalList& My_Mesh::get_normals()
{
	return this->m_normals_;
};

const FaceList&   My_Mesh::get_faces()
{
	return this->m_faces_;
};

int My_Mesh::num_faces1()
{
	if(m_faces_.size()%9==0)
	return this->m_faces_.size()/9;

	else return this->m_faces_.size()/3;		//3
};

int My_Mesh::mode(){
	if(m_faces_.size()%9==0)
	return 1;
	else return 0;
}

int My_Mesh::num_vertices()
{
	return this->m_vertices_.size()/3;
};

const point3f& My_Mesh::get_center()
{
	return this->m_center_;
};
// 获得正方形的每个角度
double getSquareAngle(int point)
{
	return M_PI / 4 + (M_PI / 2 * point);
}
double walls_scale = 500;

//前后墙壁
void My_Mesh::generate_wall_fb(int num_division)
{
	this->clear_data();
	this->m_center_ = point3f(0, 0, 0);
	this->m_min_box_ = point3f(-1, -1, 0);
	this->m_max_box_ = point3f(1, 1, 0);

	int num_samples = num_division;

	double currentAngle;

	int z = sin(getSquareAngle(2))* walls_scale;
	for (int i = 0; i < num_samples; i++)
	{
		currentAngle = getSquareAngle(i);
		int x = cos(-currentAngle)* walls_scale;
		int y = abs(sin(currentAngle)* walls_scale) + sin(currentAngle)* walls_scale;

		m_vertices_.push_back(x);
		m_vertices_.push_back(y);
		m_vertices_.push_back(z);

		m_normals_.push_back(x);
		m_normals_.push_back(y);
		m_normals_.push_back(0);
		//法线由里向外
		float r;
		float g;
		float b;
		My_Mesh::normal_to_color(x, y, z, r, g, b);
		//这里采用法线来生成颜色
		m_color_list_.push_back(r);
		m_color_list_.push_back(g);
		m_color_list_.push_back(b);
	}
	m_vertices_.push_back(0);
	m_vertices_.push_back(0);
	m_vertices_.push_back(0);
	m_normals_.push_back(0);
	m_normals_.push_back(0);
	m_normals_.push_back(1);
	//法线由里向外
	float r;
	float g;
	float b;
	My_Mesh::normal_to_color(0, 0, 1, r, g, b);
	m_color_list_.push_back(r);
	m_color_list_.push_back(g);
	m_color_list_.push_back(b);

	//////////////////////////////////////////
	//生成三角面片
	m_faces_.push_back(0);
	m_faces_.push_back(1);
	m_faces_.push_back(2);
	//生成三角面片		
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(1);
	//纹理坐标

	m_vt_list_.push_back(1);
	m_vt_list_.push_back(1);
	//纹理坐标

	m_vt_list_.push_back(1);
	m_vt_list_.push_back(0);
	//纹理坐标
	//////////////////////////////////////
	m_faces_.push_back(0);
	m_faces_.push_back(2);
	m_faces_.push_back(3);
	//生成三角面片		
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(1);
	//纹理坐标

	m_vt_list_.push_back(1);
	m_vt_list_.push_back(0);
	//纹理坐标

	m_vt_list_.push_back(0);
	m_vt_list_.push_back(0);
	//纹理坐标

};

//左右墙壁
void My_Mesh::generate_wall_lr(int num_division)
{
	this->clear_data();
	this->m_center_ = point3f(0, 0, 0);
	this->m_min_box_ = point3f(-1, -1, 0);
	this->m_max_box_ = point3f(1, 1, 0);

	int num_samples = num_division;

	double currentAngle;

	int x = sin(getSquareAngle(2))* walls_scale;
	for (int i = 0; i < num_samples; i++)
	{
		currentAngle = getSquareAngle(i);
		int z = cos(-currentAngle)* walls_scale;
		int y = abs(sin(currentAngle)* walls_scale) + sin(currentAngle)* walls_scale;

		m_vertices_.push_back(x);
		m_vertices_.push_back(y);
		m_vertices_.push_back(z);

		m_normals_.push_back(x);
		m_normals_.push_back(y);
		m_normals_.push_back(0);
		//法线由里向外
		float r;
		float g;
		float b;
		My_Mesh::normal_to_color(x, y, z, r, g, b);
		m_color_list_.push_back(r);
		m_color_list_.push_back(g);
		m_color_list_.push_back(b);
	}
	m_vertices_.push_back(0);
	m_vertices_.push_back(0);
	m_vertices_.push_back(0);
	m_normals_.push_back(0);
	m_normals_.push_back(0);
	m_normals_.push_back(1);
	//法线由里向外
	float r;
	float g;
	float b;
	My_Mesh::normal_to_color(0, 0, 1, r, g, b);
	//这里采用法线来生成颜色，学生可以自定义自己的颜色生成方式
	m_color_list_.push_back(r);
	m_color_list_.push_back(g);
	m_color_list_.push_back(b);

	//////////////////////////////////////////
	//生成三角面片
	m_faces_.push_back(0);
	m_faces_.push_back(1);
	m_faces_.push_back(2);
	//生成三角面片		
	m_vt_list_.push_back(1);
	m_vt_list_.push_back(1);
	//纹理坐标
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(1);
	//纹理坐标
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(0);
	//纹理坐标
	//////////////////////////////////////
	m_faces_.push_back(0);
	m_faces_.push_back(2);
	m_faces_.push_back(3);
	//生成三角面片		
	m_vt_list_.push_back(1);
	m_vt_list_.push_back(1);
	//纹理坐标
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(0);
	//纹理坐标
	m_vt_list_.push_back(1);
	m_vt_list_.push_back(0);
	//纹理坐标
};

// 生成地面
void My_Mesh::generate_floor(int num_division)
{
	this->clear_data();
	this->m_center_ = point3f(0, 0, 0);
	this->m_min_box_ = point3f(-1, -1, 0);
	this->m_max_box_ = point3f(1, 1, 0);
	double currentAngle;

	int num_samples = num_division;	// 点的总数
	int y = 0;	// 因为是平面，所以所有坐标的y都为定值，这里设置为0 
	for (int i = 0; i < num_samples; i++)
	{
		// 获取正方形四个角的角度
		currentAngle = getSquareAngle(i);
		// 用角度计算x和z的坐标
		int x = cos(-currentAngle)* walls_scale;
		int z = sin(currentAngle)* walls_scale;

		m_vertices_.push_back(x);
		m_vertices_.push_back(y);
		m_vertices_.push_back(z);

		m_normals_.push_back(x);
		m_normals_.push_back(y);
		m_normals_.push_back(0);
		//法线由里向外
		float r;
		float g;
		float b;
		My_Mesh::normal_to_color(x, y, z, r, g, b);
		m_color_list_.push_back(r);
		m_color_list_.push_back(g);
		m_color_list_.push_back(b);
	}
	m_vertices_.push_back(0);
	m_vertices_.push_back(0);
	m_vertices_.push_back(0);
	m_normals_.push_back(0);
	m_normals_.push_back(0);
	m_normals_.push_back(1);
	//法线由里向外
	float r;
	float g;
	float b;
	My_Mesh::normal_to_color(0, 0, 1, r, g, b);
	//这里采用法线来生成颜色，学生可以自定义自己的颜色生成方式
	m_color_list_.push_back(r);
	m_color_list_.push_back(g);
	m_color_list_.push_back(b);

	//////////////////////////////////////////
	//生成三角面片
	m_faces_.push_back(0);
	m_faces_.push_back(1);
	m_faces_.push_back(2);

	//纹理坐标
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(0);

	m_vt_list_.push_back(1);
	m_vt_list_.push_back(0);

	m_vt_list_.push_back(1);
	m_vt_list_.push_back(1);
	//////////////////////////////////////
	//生成三角面片		
	m_faces_.push_back(0);
	m_faces_.push_back(2);
	m_faces_.push_back(3);

	//纹理坐标
	m_vt_list_.push_back(0);
	m_vt_list_.push_back(0);

	m_vt_list_.push_back(1);
	m_vt_list_.push_back(1);

	m_vt_list_.push_back(0);
	m_vt_list_.push_back(1);

};

void My_Mesh::set_texture_file(std::string s)
{
	this->texture_file_name = s;
};

std::string My_Mesh::get_texture_file()
{
	return this->texture_file_name;
};

void My_Mesh::set_translate(float x, float y, float z)
{
	vTranslation[0] = x;
	vTranslation[1] = y;
	vTranslation[2] = z;

};
void My_Mesh::get_translate(float& x, float& y, float& z)
{
	x = vTranslation[0];
	y = vTranslation[1];
	z = vTranslation[2];
};

void My_Mesh::set_theta(float x, float y, float z)
{
	Theta[0] = x;
	Theta[1] = y;
	Theta[2] = z;
};
void My_Mesh::get_theta(float& x, float& y, float& z)
{
	x = Theta[0];
	y = Theta[1];
	z = Theta[2];
};

// void My_Mesh::set_theta_step(float x, float y, float z)
// {
// 	Theta_step[0] = x;
// 	Theta_step[1] = y;
// 	Theta_step[2] = z;
// };

// void My_Mesh::add_theta_step()
// {
// 	Theta[0] = Theta[0] + Theta_step[0];
// 	Theta[1] = Theta[1] + Theta_step[1];
// 	Theta[2] = Theta[2] + Theta_step[2];
// };