#include "Mesh_Painter.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include<iostream>
using namespace std;
#include<stack>


Mesh_Painter::Mesh_Painter()
{
}

Mesh_Painter::~Mesh_Painter()
{
}

GLuint modelViewMatrixID;
mat4 modelMatrix;
mat4 viewMatrix;
mat4 projMatrix;

void Mesh_Painter::setCameraMatrix(mat4 model, mat4 view, mat4 pro)
{
	modelMatrix = model;
	viewMatrix = view;
	projMatrix = pro;
}

void Mesh_Painter::draw_meshes()
{

    for (unsigned int i = 0; i < this->m_my_meshes_.size(); i++)
    { 
        
        glUseProgram(this->program_all[i]);
        
        glBindVertexArray(this->vao_all[i]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->textures_all[i]);// 该语句必须，否则将只使用同一个纹理进行绘制
            
        float x, y, z;
        this->m_my_meshes_[i]->get_theta(x, y, z);
        GLfloat  Theta[3] = {x, y, z};
        glUniform3fv(theta_all[i], 1, Theta);

        //平移
        this->m_my_meshes_[i]->get_translate(x, y, z);
        GLfloat  vTranslation[3] = { x, y, z };
        glUniform3fv(trans_all[i], 1, vTranslation);

        modelViewMatrixID = glGetUniformLocation(this->program_all[i], "modelViewMatrix");

		mat4 modelViewMatrix = projMatrix * viewMatrix * modelMatrix;
		glUniformMatrix4fv(modelViewMatrixID, 1, GL_TRUE, &modelViewMatrix[0][0]);
        
        glDrawArrays(GL_TRIANGLES, 0, this->m_my_meshes_[i]->num_faces1() * 3);
        glUseProgram(0);
    }
};

void Mesh_Painter::update_texture()
{
    this->textures_all.clear();

    for (unsigned int i = 0; i < this->m_my_meshes_.size(); i++)
    {
        GLuint textures;

        // 调用stb_image生成纹理
        glGenTextures(1, &textures);
        load_texture_STBImage(this->m_my_meshes_[i]->get_texture_file(), textures);

        // 将生成的纹理传给shader
        glBindTexture(GL_TEXTURE_2D, textures);
        glUniform1i(glGetUniformLocation(this->program_all[i], "texture"), 0);
        this->textures_all.push_back(textures);
    }
};

void Mesh_Painter::load_texture_STBImage(std::string file_name, GLuint& m_texName)
{
    int width, height, channels = 0;
    unsigned char *pixels = NULL;
    stbi_set_flip_vertically_on_load(true);

    pixels = stbi_load(file_name.c_str(), &width, &height, &channels, 0);

    // 调整行对齐格式
    if(width*channels%4!=0) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLenum format = GL_RGB;
    // 设置通道格式
    switch (channels){
    case 1: format=GL_RED;break;
    case 3: format=GL_RGB;break;
    case 4: format=GL_RGBA;break;
    default: format=GL_RGB;break;
    }

    // 绑定纹理对象
    glBindTexture(GL_TEXTURE_2D, m_texName);

    // 指定纹理的放大，缩小滤波，使用线性方式，即当图片放大的时候插值方式
    // 将图片的rgb数据上传给opengl
    glTexImage2D(
        GL_TEXTURE_2D,  // 指定目标纹理，这个值必须是GL_TEXTURE_2D
        0,              // 执行细节级别，0是最基本的图像级别，n表示第N级贴图细化级别
        format,         // 纹理数据的颜色格式(GPU显存)
        width,          // 宽度。早期的显卡不支持不规则的纹理，则宽度和高度必须是2^n
        height,         // 高度。早期的显卡不支持不规则的纹理，则宽度和高度必须是2^n
        0,              // 指定边框的宽度。必须为0
        format,         // 像素数据的颜色格式(CPU内存)
        GL_UNSIGNED_BYTE,   // 指定像素数据的数据类型
        pixels          // 指定内存中指向图像数据的指针
    );
    
    // 生成多级渐远纹理，多消耗1/3的显存，较小分辨率时获得更好的效果
    // glGenerateMipmap(GL_TEXTURE_2D);

    // 指定插值方法
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // 恢复初始对齐格式
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    // 释放图形内存
    stbi_image_free(pixels);
};

void Mesh_Painter::update_vertex_buffer()
{
    this->vao_all.clear();
    this->buffer_all.clear();
    this->vPosition_all.clear();
    this->vColor_all.clear();
    this->vTexCoord_all.clear();
    this->vNormal_all.clear();

    // 顶点坐标，法线，颜色，纹理坐标到shader的映射
    for (unsigned int m_i = 0; m_i < this->m_my_meshes_.size(); m_i++)
    {
        int num_face = this->m_my_meshes_[m_i]->num_faces1();
        int num_vertex = this->m_my_meshes_[m_i]->num_vertices();

        const VertexList& vertex_list = this->m_my_meshes_[m_i]->get_vertices();
        const NormalList& normal_list = this->m_my_meshes_[m_i]->get_normals();
        const FaceList&  face_list = this->m_my_meshes_[m_i]->get_faces();
        const STLVectorf& color_list = this->m_my_meshes_[m_i]->get_colors();
        const VtList& vt_list = this->m_my_meshes_[m_i]->get_vts();

        // 创建顶点数组对象
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(vec3)*num_face * 3
            + sizeof(vec3)*num_face * 3
            + sizeof(vec3)*num_face * 3
            + sizeof(vec2)*num_face * 3, NULL, GL_STATIC_DRAW);

        // ------------------------------------------------------------------------
        // 获得足够的空间存储坐标，颜色，法线以及纹理坐标等，并将它们映射给shader
        // Specify an offset to keep track of where we're placing data in our
        // vertex array buffer.  We'll use the same technique when we
        // associate the offsets with vertex attribute pointers.
        GLintptr offset = 0;
        point3f  p_center_ = this->m_my_meshes_[m_i]->get_center();
        point3f p_min_box_, p_max_box_;
        this->m_my_meshes_[m_i]->get_boundingbox(p_min_box_, p_max_box_);
        float d = p_min_box_.distance(p_max_box_);

if(this->m_my_meshes_[m_i]->mode()){
        // -------------------- 实现顶点到shader的映射 ------------------------------
        vec3* points = new vec3[num_face * 3];
        for (int i = 0; i < num_face; i++)
        {
            // @TODO: 参考实验4.1，在这里添加代码实现顶点坐标到shader的映射
            int index = 3*(face_list[9*i]-1);
            points[3 * i] = vec3(
                                (vertex_list[index] - p_center_.x) / (1.5 * d),
                                (vertex_list[index+1] - p_center_.y) / (1.5 * d),
                                (vertex_list[index+2] - p_center_.z) / (1.5 * d)
                                );
            index = 3*(face_list[9*i+3]-1);
            points[3 * i + 1] = vec3(
                                    (vertex_list[index] - p_center_.x) / (1.5 * d),
                                    (vertex_list[index+1] - p_center_.y) / (1.5 * d),
                                    (vertex_list[index+2] - p_center_.z) / (1.5 * d)
                                    );
            index = 3*(face_list[9*i+6]-1);
            points[3 * i + 2] = vec3(
                                    (vertex_list[index] - p_center_.x) / (1.5 * d),
                                    (vertex_list[index+1] - p_center_.y) / (1.5 * d),
                                    (vertex_list[index+2] - p_center_.z) / (1.5 * d)
                                    );
        }
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec3)*num_face * 3, points);
        offset += sizeof(vec3) * num_face * 3;
        delete[] points;
        // ------------------------------------------------------------------------
        
        // -------------------- 实现法线到shader的映射 ------------------------------
        points = new vec3[num_face * 3];
        for (int i = 0; i < num_face; i++)
        {
            // @TODO: 参考实验4.1，在这里添加代码实现法线到shader的映射
            int index = 3*(face_list[9*i+2]-1);
            points[3 * i] = vec3(
                                (normal_list[index]),
                                (normal_list[index+1]),
                                (normal_list[index+2])
                                );
            index = 3*(face_list[9*i+3+2]-1);
            points[3 * i + 1] = vec3(
                                    (normal_list[index]),
                                    (normal_list[index+1]),
                                    (normal_list[index+2])
                                    );
            index = 3*(face_list[9*i+6+2]-1);
            points[3 * i + 2] = vec3(
                                    (normal_list[index]),
                                    (normal_list[index+1]),
                                    (normal_list[index+2])
                                    );
        }
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec3) * num_face * 3, points);
        offset += sizeof(vec3) * num_face * 3;
        delete[] points;
        // ------------------------------------------------------------------------

        // -------------------- 实现颜色到shader的映射 ------------------------------
        points = new vec3[num_face * 3];
        for (int i = 0; i < num_face; i++)
        {
            // @TODO: 参考实验4.1，在这里添加代码实现颜色到shader的映射
            int index = 3*(face_list[9*i+2]-1);
            points[3 * i] = vec3((color_list[index+0]),
                                (color_list[index+1]),
                                (color_list[index+2]));
            index = 3*(face_list[9*i+3+2]-1);
            points[3 * i + 1] = vec3((color_list[index+0]),
                                    (color_list[index+1]),
                                    (color_list[index+2]));
            index = 3*(face_list[9*i+6+2]-1);
            points[3 * i + 2] = vec3((color_list[index+0]),
                                    (color_list[index+1]),
                                    (color_list[index+2]));
        }
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec3) * num_face * 3, points);
        offset += sizeof(vec3) * num_face * 3;
        delete[] points;
        // ------------------------------------------------------------------------

        // ------------------- 实现纹理坐标到shader的映射 -----------------------------
        vec2* points_2 = new vec2[num_face * 3];
        for (int i = 0; i < num_face; i++)
        {
            // @TODO: 参考实验4.1，在这里添加代码实现纹理坐标到shader的映射
            int index = 3*(face_list[9*i+1]-1);
            points_2[i * 3] = vec2(vt_list[index+0], vt_list[index+1]);
            index = 3*(face_list[9*i+3+1]-1);
            points_2[i * 3 + 1] = vec2(vt_list[index+0], vt_list[index+1]);
            index = 3*(face_list[9*i+6+1]-1);
            points_2[i * 3 + 2] = vec2(vt_list[index+0], vt_list[index+1]);
        }
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec2) * num_face * 3, points_2);
        offset += sizeof(vec2) * num_face * 3;
        delete[] points_2;
}
else{
    // -------------------- 实现顶点到shader的映射 ------------------------------
		vec3* points = new vec3[num_face * 3];
		for (int i = 0; i < num_face; i++)
		{
			int index = face_list[3 * i];
			points[3 * i] = vec3((vertex_list[index * 3 + 0] - p_center_.x) / (1.5 * d),
								(vertex_list[index * 3 + 1] - p_center_.y) / (1.5 * d),
								(vertex_list[index * 3 + 2] - p_center_.z) / (1.5 * d));
			index = face_list[3 * i + 1];
			points[3 * i + 1] = vec3((vertex_list[index * 3 + 0] - p_center_.x) / (1.5 * d),
									(vertex_list[index * 3 + 1] - p_center_.y) / (1.5 * d),
									(vertex_list[index * 3 + 2] - p_center_.z) / (1.5 * d));
			index = face_list[3 * i + 2];
			points[3 * i + 2] = vec3((vertex_list[index * 3 + 0] - p_center_.x) / (1.5 * d),
									(vertex_list[index * 3 + 1] - p_center_.y) / (1.5 * d),
									(vertex_list[index * 3 + 2] - p_center_.z) / (1.5 * d));
		}
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec3) * num_face * 3, points);
		offset += sizeof(vec3) * num_face * 3;
		delete[] points;
		// ------------------------------------------------------------------------
		
		// -------------------- 实现法线到shader的映射 ------------------------------
		points = new vec3[num_face * 3];
		for (int i = 0; i < num_face; i++)
		{
			int index = face_list[3 * i];
			points[3 * i] = vec3((normal_list[index * 3 + 0]),
								(normal_list[index * 3 + 1]),
								(normal_list[index * 3 + 2]));
			index = face_list[3 * i + 1];
			points[3 * i + 1] = vec3((normal_list[index * 3 + 0]),
									(normal_list[index * 3 + 1]),
									(normal_list[index * 3 + 2]));
			index = face_list[3 * i + 2];
			points[3 * i + 2] = vec3((normal_list[index * 3 + 0]),
									(normal_list[index * 3 + 1]),
									(normal_list[index * 3 + 2]));
		}
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec3) * num_face * 3, points);
		offset += sizeof(vec3) * num_face * 3;
		delete[] points;
		// ------------------------------------------------------------------------

		// -------------------- 实现颜色到shader的映射 ------------------------------
		points = new vec3[num_face * 3];
		for (int i = 0; i < num_face; i++)
		{
			int index = face_list[3 * i];
			points[3 * i] = vec3((color_list[index * 3 + 0]),
								(color_list[index * 3 + 1]),
								(color_list[index * 3 + 2]));
			index = face_list[3 * i + 1];
			points[3 * i + 1] = vec3((color_list[index * 3 + 0]),
									(color_list[index * 3 + 1]),
									(color_list[index * 3 + 2]));
			index = face_list[3 * i + 2];
			points[3 * i + 2] = vec3((color_list[index * 3 + 0]),
									(color_list[index * 3 + 1]),
									(color_list[index * 3 + 2]));
		}
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec3) * num_face * 3, points);
		offset += sizeof(vec3) * num_face * 3;
		delete[] points;
		// ------------------------------------------------------------------------

		// ------------------- 实现纹理坐标到shader的映射 -----------------------------
		vec2* points_2 = new vec2[num_face * 3];
		for (int i = 0; i < num_face; i++)
		{
			points_2[i * 3] = vec2(vt_list[i * 6 + 0], vt_list[i * 6 + 1]);
			points_2[i * 3 + 1] = vec2(vt_list[i * 6 + 2], vt_list[i * 6 + 3]);
			points_2[i * 3 + 2] = vec2(vt_list[i * 6 + 4], vt_list[i * 6 + 5]);
		}
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vec2) * num_face * 3, points_2);
		offset += sizeof(vec2) * num_face * 3;
		delete[] points_2;
}
        // ------------------------------------------------------------------------

        // 加载着色器并使用生成的着色器程序
        offset = 0;
        // 指定vPosition在shader中使用时的位置
        GLuint vPosition;
        vPosition = glGetAttribLocation(this->program_all[m_i], "vPosition");
        glEnableVertexAttribArray(vPosition);
        glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(offset));
        offset += sizeof(vec3) * num_face * 3;

        // 指定vNormal在shader中使用时的位置
        GLuint vNormal;
        vNormal = glGetAttribLocation(this->program_all[m_i], "vNormal");
        glEnableVertexAttribArray(vNormal);
        glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(offset));
        offset += sizeof(vec3) * num_face * 3;
        
        // 指定vColor在shader中使用时的位置
        GLuint vColor;
        vColor = glGetAttribLocation(this->program_all[m_i], "vColor");
        glEnableVertexAttribArray(vColor);
        glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(offset));
        offset += sizeof(vec3) * num_face * 3;
        
        //指定vTexCoord在shader中使用时的位置
        GLuint vTexCoord;
        vTexCoord = glGetAttribLocation(this->program_all[m_i], "vTexCoord");
        glEnableVertexAttribArray(vTexCoord);
        glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(offset));
        
        this->vao_all.push_back(vao);
        this->buffer_all.push_back(buffer);
        this->vPosition_all.push_back(vPosition);
        this->vColor_all.push_back(vColor);
        this->vTexCoord_all.push_back(vTexCoord);
        this->vNormal_all.push_back(vNormal);
    }
};

void Mesh_Painter::init_shaders(std::string vs, std::string fs)
{
    this->program_all.clear();
    this->theta_all.clear();
    this->trans_all.clear();

    for (unsigned int i = 0; i < this->m_my_meshes_.size(); i++)
    {
        GLuint program = InitShader(vs.c_str(), fs.c_str());
        this->program_all.push_back(program);

        GLuint  theta = glGetUniformLocation(program, "theta");
        GLuint  trans = glGetUniformLocation(program, "translation");

        theta_all.push_back(theta);
        trans_all.push_back(trans);
    }

};

void Mesh_Painter::add_mesh(My_Mesh* m)
{
    this->m_my_meshes_.push_back(m);
};

void Mesh_Painter::clear_mehs()
{
    this->m_my_meshes_.clear();

    this->textures_all.clear();
    this->program_all.clear();
    this->vao_all.clear();
    this->buffer_all.clear();
    this->vPosition_all.clear();
    this->vColor_all.clear();
    this->vTexCoord_all.clear();
    this->vNormal_all.clear();
};


