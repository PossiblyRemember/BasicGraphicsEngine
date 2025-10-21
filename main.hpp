// basic_game_LOVE_CMAKE_SMILE.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#define GLM_ENABLE_EXPERIMENTAL


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <cmath>
#include <print>

using glm::vec3;
using std::vector;

namespace Game{
	struct Transform {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(glm::vec3(0.0f)); // from Euler or direct quaternion
        glm::vec3 scale = glm::vec3(1.0f);
	};
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
	};

	class GameObject {
	public:
		Transform transform; // Transform for position, rotation, and scale

		static glm::vec3 ConvertQuatToEuler(const glm::quat& q) {
            // Convert quaternion to Euler angles (in radians)
            float roll = atan2(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));
            float pitch = asin(2.0f * (q.w * q.y - q.z * q.x));
            float yaw = atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
            return glm::vec3(pitch, yaw, roll);
        }
		static glm::quat ConvertEulerToQuat(const glm::vec3& euler) {
            // Convert Euler angles (in radians) to quaternion
            float cr = cos(euler.x * 0.5f);
            float sr = sin(euler.x * 0.5f);
            float cp = cos(euler.y * 0.5f);
            float sp = sin(euler.y * 0.5f);
            float cy = cos(euler.z * 0.5f);
            float sy = sin(euler.z * 0.5f);
            glm::quat q;
            q.w = cr * cp * cy + sr * sp * sy;
            q.x = sr * cp * cy - cr * sp * sy;
            q.y = cr * sp * cy + sr * cp * sy;
            q.z = cr * cp * sy - sr * sp * cy;
            return q;
        }

        GameObject() {
            transform.position = glm::vec3(0.0f);
            transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            transform.scale = glm::vec3(1.0f);
        }
        virtual ~GameObject() {}

	private:

	};

    class Geometry : public Game::GameObject {
    public:
        vector<vec3> vertices;
        vector<unsigned int> indices;

        GLuint vao = 0, vbo = 0, ebo = 0;


        Geometry() {
        }

        Geometry(const std::string& path) {

            Assimp::Importer importer; // initalize model importer and process model
            const aiScene* scene = importer.ReadFile(path,
                aiProcess_Triangulate |
                aiProcess_GenNormals |
                aiProcess_FlipUVs |
                aiProcess_JoinIdenticalVertices);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
                return;
            }

            for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
                aiMesh* mesh = scene->mMeshes[m];
                size_t vertexOffset = vertices.size(); // prior vertices count
                for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                    vertices.push_back(vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
                }

                // Indices (faces)
                for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                    aiFace face = mesh->mFaces[i];
                    for (unsigned int j = 0; j < face.mNumIndices; j++) {
                        indices.push_back(face.mIndices[j] + static_cast<unsigned int>(vertexOffset));
                    }
                }
            }

            upload();
        }

        Geometry(const std::string& path, Transform initTransform) {
            transform = initTransform;
            Geometry::Geometry(path);
        }


        virtual ~Geometry() {
            if (ebo) glDeleteBuffers(1, &ebo);
            if (vbo) glDeleteBuffers(1, &vbo);
            if (vao) glDeleteVertexArrays(1, &vao);


        }


        glm::mat4 getModelMatrix() const {
            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, transform.position);

            // Rotation
            model *= glm::toMat4(transform.rotation);


            model = glm::scale(model, transform.scale);

            return model;
        }

        void upload() {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);

            // Upload vertex data 
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);

            if (!indices.empty()) {
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
            }

            /*glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);*/

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);


            glBindVertexArray(0);
        }

        void draw(GLuint shaderProgram) {
            glUseProgram(shaderProgram);

            glm::mat4 model = getModelMatrix();

            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(vao);

            if (!indices.empty()) {
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            }
            else {
                // vertices.size() is the number of `vec3` vertices
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
            }

            glBindVertexArray(0);
        }
    };

    class Cube : public Geometry {
    public:
        Cube(float scale = 1) {
            vertices = {
                // positions         
               {-0.5f*scale, -0.5f*scale, -0.5f*scale},
               { 0.5f*scale, -0.5f*scale, -0.5f*scale},
               { 0.5f*scale,  0.5f*scale, -0.5f*scale},
               {-0.5f*scale,  0.5f*scale, -0.5f*scale},
               {-0.5f*scale, -0.5f*scale,  0.5f*scale},
               { 0.5f*scale, -0.5f*scale,  0.5f*scale},
               { 0.5f*scale,  0.5f*scale,  0.5f*scale},
               {-0.5f*scale,  0.5f*scale,  0.5f*scale}
            };

            indices = {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4,
                0, 1, 5, 5, 4, 0,
                2, 3, 7, 7, 6, 2,
                1, 2, 6, 6, 5, 1,
                3, 0, 4, 4, 7, 3
            };

            upload();
        }
    };
}