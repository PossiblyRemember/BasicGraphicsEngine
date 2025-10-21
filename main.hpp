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
#pragma region HelperFunctions
    glm::quat lookAtQuaternion(const glm::vec3& fromDir, const glm::vec3& toTarget, const glm::vec3& up = glm::vec3(0, 1, 0)) {
        glm::vec3 forward = glm::normalize(toTarget - fromDir);
        glm::vec3 right = glm::normalize(glm::cross(up, forward));
        glm::vec3 newUp = glm::cross(forward, right);

        glm::mat3 rotMat(right, newUp, forward); // columns = right, up, forward
        return glm::quat_cast(rotMat);
    }

    class Shader {
    public:
        GLuint ID;

        Shader(const char* vertexSrc, const char* fragmentSrc) {
            // compile shaders, link program, store ID
            // error handling omitted for brevity
            GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vertexSrc, nullptr);
            glCompileShader(vertex);

            GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fragmentSrc, nullptr);
            glCompileShader(fragment);

            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            glLinkProgram(ID);

            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }

        void use() const {
            glUseProgram(ID);
        }

        void setInt(const std::string& name, int value) const {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }

        void setVec3(const std::string& name, const glm::vec3& value) const {
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }

        void setMat4(const std::string& name, const glm::mat4& mat) const {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
    };
#pragma endregion

#pragma region BaseProperties
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
#pragma endregion

#pragma region EngineObjects
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

        GameObject() = default;
        GameObject(const Transform& transform) : transform(transform) {
        }
        virtual ~GameObject() {}

	private:

	};

    class Geometry : public Game::GameObject {
    public:
        vector<Vertex> vertices;
        vector<unsigned int> indices;

        GLuint vao = 0, vbo = 0, ebo = 0;
        GLuint textureID = 0; 


        Geometry() {
        }

        Geometry(const std::string& path) {
            createDefaultWhiteTexture(); // debug

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

            for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                size_t vertexOffset = vertices.size(); // number of vertices already in the vector

                for (unsigned int i = 0; i < scene->mMeshes[m]->mNumVertices; ++i) {
                    glm::vec3 pos(scene->mMeshes[m]->mVertices[i].x, scene->mMeshes[m]->mVertices[i].y, scene->mMeshes[m]->mVertices[i].z);
                    glm::vec3 norm(scene->mMeshes[m]->mNormals[i].x, scene->mMeshes[m]->mNormals[i].y, scene->mMeshes[m]->mNormals[i].z);

                    glm::vec2 uv(0.0f, 0.0f);
                    if (scene->mMeshes[m]->mTextureCoords[0]) {
                        uv = glm::vec2(scene->mMeshes[m]->mTextureCoords[0][i].x, scene->mMeshes[m]->mTextureCoords[0][i].y);
                    }

                    vertices.push_back({ pos, norm, uv });
                }

                for (unsigned int i = 0; i < scene->mMeshes[m]->mNumFaces; ++i) {
                    aiFace face = scene->mMeshes[m]->mFaces[i];
                    for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                        indices.push_back(face.mIndices[j] + vertexOffset);
                    }
                }
            }
            if (!indices.empty()) {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
            else {
                glDrawArrays(GL_TRIANGLES, 0, vertices.size());
            }

            upload();
        }

        Geometry(const std::string& path, const Transform& initTransform) : Geometry(path) {transform = initTransform;}


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

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

            if (!indices.empty()) {
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
            }

            // --- Vertex attributes ---
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0); // position
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // normal
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords)); // uv
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
        }


        void draw(GLuint shaderProgram) {
            glUseProgram(shaderProgram);

            glm::mat4 model = getModelMatrix();

            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0); // shader uniform

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

    private:
        void createDefaultWhiteTexture() {
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            unsigned char whitePixel[3] = { 255, 255, 255 }; // RGB
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

    };

#pragma region Lights
    struct DirectionalLight {
        vec3 direction;
        vec3 color;
    };

    class LightManager {
    public:
        std::vector<DirectionalLight> dirLights;

        void addDirectionalLight(const glm::vec3& direction, const glm::vec3& color) {
            dirLights.push_back({ direction, color });
        }

        void pointLightAtTarget(size_t index, const glm::vec3& from, const glm::vec3& target) {
            if (index >= dirLights.size()) return;
            glm::quat q = lookAtQuaternion(from, target);
            dirLights[index].direction = glm::rotate(q, glm::vec3(0, 0, -1));
        }

        void uploadToShader(Shader& shader) {
            shader.use();
            shader.setInt("numDirLights", static_cast<int>(dirLights.size()));
            for (size_t i = 0; i < dirLights.size(); ++i) {
                std::string base = "dirLights[" + std::to_string(i) + "]";
                shader.setVec3(base + ".direction", dirLights[i].direction);
                shader.setVec3(base + ".color", dirLights[i].color);
            }
        }
    };
#pragma endregion
#pragma endregion


}