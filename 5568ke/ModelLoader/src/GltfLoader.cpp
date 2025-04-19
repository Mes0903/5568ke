#include "GltfLoader.hpp"

#include "include_5568ke.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>

#include "BlinnPhongMaterial.hpp"
#include "BoundingBox.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Texture.hpp"

// Main method to load a GLTF model
Model* GltfLoader::loadModel(std::string const& path) { return loadGltf(path, MaterialType::BlinnPhong); }

// Calculate the center point of a model based on its bounding box
glm::vec3 GltfLoader::calculateModelCenter(Model* model)
{
	if (!model || model->boundingBoxes.empty()) {
		return glm::vec3(0.0f);
	}

	// Use the global bounding box to calculate center
	BoundingBox& bbox = model->globalBoundingBox;
	glm::vec3 center = (bbox.min + bbox.max) * 0.5f;

	// Log information
	std::cout << "[GltfLoader]  Model center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;

	return center;
}

// Calculate an appropriate scale factor for a model
float GltfLoader::calculateModelScale(Model* model, float targetSize)
{
	if (!model || model->boundingBoxes.empty()) {
		return 1.0f;
	}

	// Use the global bounding box to calculate size
	BoundingBox& bbox = model->globalBoundingBox;
	glm::vec3 size = bbox.max - bbox.min;
	float maxDim = std::max(std::max(size.x, size.y), size.z);

	// Calculate scale factor to achieve target size
	float scaleFactor = targetSize / maxDim;

	// Log information
	std::cout << "[GltfLoader]  Model size: (" << size.x << ", " << size.y << ", " << size.z << ")" << std::endl;
	std::cout << "[GltfLoader]  Using scale factor: " << scaleFactor << std::endl;

	return scaleFactor;
}

// Apply positioning to a model
void GltfLoader::positionModel(Model* model, glm::vec3 position, glm::vec3 rotation, float scale)
{
	if (!model)
		return;

	// Store the transformation parameters
	model->defaultTranslation = position;
	model->defaultRotation = rotation;
	model->defaultScale = scale;
}

// Suggest a good camera position based on model size and orientation
glm::vec3 GltfLoader::suggestCameraPosition(Model* model)
{
	if (!model || model->boundingBoxes.empty()) {
		return glm::vec3(0.0f, 1.0f, 3.0f); // Default position
	}

	// Use the global bounding box
	BoundingBox& bbox = model->globalBoundingBox;
	glm::vec3 center = (bbox.min + bbox.max) * 0.5f;
	glm::vec3 size = bbox.max - bbox.min;

	// Position camera to view the front of the model
	return glm::vec3(0.0f, center.y, center.z + size.z * 2.0f);
}

// Main GLTF loading implementation
Model* GltfLoader::loadGltf(std::string const& path, MaterialType type)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	// Enable verbose debug output
	loader.SetStoreOriginalJSONForExtrasAndExtensions(true);

	bool ret;
	// Determine file type (GLTF or GLB) and load accordingly
	if (path.find(".glb") != std::string::npos) {
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
	}
	else {
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
	}

	// Handle loading errors
	if (!warn.empty()) {
		std::cout << "[GltfLoader]  GLTF warning: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << "[GltfLoader]  GLTF error: " << err << std::endl;
		return nullptr;
	}

	if (!ret) {
		std::cerr << "[GltfLoader]  Failed to load GLTF file: " << path << std::endl;
		return nullptr;
	}

	// Create and populate model
	Model* model = new Model();

	std::cout << "[GltfLoader]  GLTF file has " << gltfModel.meshes.size() << " meshes, " << gltfModel.textures.size() << " textures, "
						<< gltfModel.materials.size() << " materials" << std::endl;

	// Process all meshes in the GLTF file
	for (tinygltf::Mesh& mesh : gltfModel.meshes) {
		Mesh outMesh;
		processMesh(gltfModel, mesh, outMesh, type);

		// Setup OpenGL buffers and VAO
		outMesh.setup();

		// Calculate bounding box
		BoundingBox bbox = calculateBoundingBox(outMesh);
		model->boundingBoxes.push_back(bbox);

		// Add mesh to model
		model->meshes.push_back(std::move(outMesh));
	}

	// Calculate global bounding box and store on the model
	if (!model->boundingBoxes.empty()) {
		model->globalBoundingBox = calculateGlobalBoundingBox(model->boundingBoxes);

		// Print global bounding box info
		std::cout << "[GltfLoader]  Model global bounding box: min(" << model->globalBoundingBox.min.x << ", " << model->globalBoundingBox.min.y << ", "
							<< model->globalBoundingBox.min.z << "), max(" << model->globalBoundingBox.max.x << ", " << model->globalBoundingBox.max.y << ", "
							<< model->globalBoundingBox.max.z << ")" << std::endl;
	}

	return model;
}

// Load texture from GLTF model
Texture* GltfLoader::loadTexture(tinygltf::Model& model, int textureIndex, TextureType type)
{
	if (textureIndex < 0)
		return nullptr;

	auto* texture = new Texture();
	texture->type = type;

	tinygltf::Texture const& gltfTexture = model.textures[textureIndex];
	tinygltf::Image const& image = model.images[gltfTexture.source];

	// Save the path for debugging/reference
	texture->path = image.uri;

	glGenTextures(1, &texture->id);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	GLenum format, internalFormat;
	if (image.component == 1) {
		format = GL_RED;
		internalFormat = GL_RED;
	}
	else if (image.component == 3) {
		format = GL_RGB;
		internalFormat = GL_RGB;
	}
	else {
		format = GL_RGBA;
		internalFormat = GL_RGBA;
	}

	// Make sure the pixel type is correct
	GLenum pixelType = GL_UNSIGNED_BYTE;
	if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
		pixelType = GL_UNSIGNED_SHORT;
	}
	else if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		pixelType = GL_FLOAT;
	}

	std::cout << "[GltfLoader]  Loading texture: " << image.uri << " (" << image.width << "x" << image.height << ", components: " << image.component << ")"
						<< std::endl;

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, pixelType, image.image.data());
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

// Create material from GLTF model
Material* GltfLoader::createMaterial(tinygltf::Model& model, tinygltf::Primitive& primitive, MaterialType type)
{
	if (type == MaterialType::BlinnPhong) {
		auto* material = new BlinnPhongMaterial();

		// Check if material exists
		if (primitive.material >= 0) {
			tinygltf::Material const& mat = model.materials[primitive.material];

			// Set base color
			if (mat.pbrMetallicRoughness.baseColorFactor.size() >= 3) {
				material->albedo =
						glm::vec3(mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);
				std::cout << "[GltfLoader]  Material albedo: " << material->albedo.x << ", " << material->albedo.y << ", " << material->albedo.z << std::endl;
			}

			// Set roughness as inverse of shininess
			if (mat.pbrMetallicRoughness.roughnessFactor >= 0) {
				// Convert roughness to shininess (inverse relationship)
				float roughness = mat.pbrMetallicRoughness.roughnessFactor;
				material->shininess = std::max(2.0f, 128.0f * (1.0f - roughness));
				std::cout << "[GltfLoader]  Material shininess: " << material->shininess << std::endl;
			}

			// Load diffuse texture
			if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
				material->diffuseMap = loadTexture(model, mat.pbrMetallicRoughness.baseColorTexture.index, TextureType::Diffuse);
				std::cout << "[GltfLoader]  Loaded diffuse texture" << std::endl;
			}

			// Check for additional textures that could be used for overlay
			if (mat.normalTexture.index >= 0) {
				material->overlayMap = loadTexture(model, mat.normalTexture.index, TextureType::Normal);
				std::cout << "[GltfLoader]  Loaded normal/overlay texture" << std::endl;
			}
		}

		return material;
	}

	// Default fallback
	return new BlinnPhongMaterial();
}

// Process mesh data from GLTF model
void GltfLoader::processMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, Mesh& outMesh, MaterialType materialType)
{
	// Process each primitive
	for (tinygltf::Primitive& primitive : mesh.primitives) {
		Primitive outPrimitive;
		outPrimitive.indexOffset = outMesh.indices.size();

		// Get position attribute
		if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
			int posAccessorIndex = primitive.attributes["POSITION"];
			tinygltf::Accessor const& accessor = model.accessors[posAccessorIndex];
			tinygltf::BufferView const& bufferView = model.bufferViews[accessor.bufferView];
			tinygltf::Buffer const& buffer = model.buffers[bufferView.buffer];

			size_t const byteOffset = accessor.byteOffset + bufferView.byteOffset;
			size_t const byteStride = accessor.ByteStride(bufferView);
			size_t const count = accessor.count;

			size_t vertexStart = outMesh.vertices.size();
			outMesh.vertices.resize(vertexStart + count);

			// Populate positions
			for (size_t i = 0; i < count; i++) {
				float const* pos = reinterpret_cast<float const*>(&buffer.data[byteOffset + i * byteStride]);
				outMesh.vertices[vertexStart + i].position = glm::vec3(pos[0], pos[1], pos[2]);
			}

			// Get normal attribute if exists
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				int normalAccessorIndex = primitive.attributes["NORMAL"];
				tinygltf::Accessor const& normalAccessor = model.accessors[normalAccessorIndex];
				tinygltf::BufferView const& normalBufferView = model.bufferViews[normalAccessor.bufferView];
				tinygltf::Buffer const& normalBuffer = model.buffers[normalBufferView.buffer];

				size_t const normalByteOffset = normalAccessor.byteOffset + normalBufferView.byteOffset;
				size_t const normalByteStride = normalAccessor.ByteStride(normalBufferView);

				for (size_t i = 0; i < count; i++) {
					float const* normal = reinterpret_cast<float const*>(&normalBuffer.data[normalByteOffset + i * normalByteStride]);
					outMesh.vertices[vertexStart + i].normal = glm::vec3(normal[0], normal[1], normal[2]);
				}
			}
			else {
				// Default normal if not provided
				for (size_t i = 0; i < count; i++) {
					outMesh.vertices[vertexStart + i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				}
			}

			// Get texcoord attribute if exists
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				int texcoordAccessorIndex = primitive.attributes["TEXCOORD_0"];
				tinygltf::Accessor const& texcoordAccessor = model.accessors[texcoordAccessorIndex];
				tinygltf::BufferView const& texcoordBufferView = model.bufferViews[texcoordAccessor.bufferView];
				tinygltf::Buffer const& texcoordBuffer = model.buffers[texcoordBufferView.buffer];

				size_t const texcoordByteOffset = texcoordAccessor.byteOffset + texcoordBufferView.byteOffset;
				size_t const texcoordByteStride = texcoordAccessor.ByteStride(texcoordBufferView);

				for (size_t i = 0; i < count; i++) {
					float const* texcoord = reinterpret_cast<float const*>(&texcoordBuffer.data[texcoordByteOffset + i * texcoordByteStride]);
					outMesh.vertices[vertexStart + i].texcoord = glm::vec2(texcoord[0], texcoord[1]);
				}
			}
			else {
				// Default texcoord if not provided
				for (size_t i = 0; i < count; i++) {
					outMesh.vertices[vertexStart + i].texcoord = glm::vec2(0.0f, 0.0f);
				}
			}

			// Process indices
			if (primitive.indices >= 0) {
				tinygltf::Accessor const& indexAccessor = model.accessors[primitive.indices];
				tinygltf::BufferView const& indexBufferView = model.bufferViews[indexAccessor.bufferView];
				tinygltf::Buffer const& indexBuffer = model.buffers[indexBufferView.buffer];

				size_t const indexByteOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
				size_t const indexCount = indexAccessor.count;

				outPrimitive.indexCount = indexCount;
				size_t indexStart = outMesh.indices.size();
				outMesh.indices.resize(indexStart + indexCount);

				// Copy indices based on component type
				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					uint16_t const* indices = reinterpret_cast<uint16_t const*>(&indexBuffer.data[indexByteOffset]);
					for (size_t i = 0; i < indexCount; i++) {
						outMesh.indices[indexStart + i] = static_cast<unsigned int>(indices[i]) + vertexStart;
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					uint32_t const* indices = reinterpret_cast<uint32_t const*>(&indexBuffer.data[indexByteOffset]);
					for (size_t i = 0; i < indexCount; i++) {
						outMesh.indices[indexStart + i] = static_cast<unsigned int>(indices[i]) + vertexStart;
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
					uint8_t const* indices = reinterpret_cast<uint8_t const*>(&indexBuffer.data[indexByteOffset]);
					for (size_t i = 0; i < indexCount; i++) {
						outMesh.indices[indexStart + i] = static_cast<unsigned int>(indices[i]) + vertexStart;
					}
				}
			}
			else {
				// If no indices are provided, generate sequential indices
				outPrimitive.indexCount = count;
				size_t indexStart = outMesh.indices.size();
				outMesh.indices.resize(indexStart + count);

				for (size_t i = 0; i < count; i++) {
					outMesh.indices[indexStart + i] = vertexStart + i;
				}
			}

			// Create material for this primitive
			outPrimitive.material = createMaterial(model, primitive, materialType);
			outMesh.primitives.push_back(outPrimitive);
		}
	}
}

// Calculate bounding box for a mesh
BoundingBox GltfLoader::calculateBoundingBox(Mesh const& mesh)
{
	BoundingBox bbox;
	if (mesh.vertices.empty()) {
		bbox.min = glm::vec3(0.0f);
		bbox.max = glm::vec3(0.0f);
		return bbox;
	}

	bbox.min = glm::vec3(std::numeric_limits<float>::max());
	bbox.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (auto const& vertex : mesh.vertices) {
		bbox.min.x = std::min(bbox.min.x, vertex.position.x);
		bbox.min.y = std::min(bbox.min.y, vertex.position.y);
		bbox.min.z = std::min(bbox.min.z, vertex.position.z);

		bbox.max.x = std::max(bbox.max.x, vertex.position.x);
		bbox.max.y = std::max(bbox.max.y, vertex.position.y);
		bbox.max.z = std::max(bbox.max.z, vertex.position.z);
	}

	return bbox;
}

// Calculate global bounding box for a model
BoundingBox GltfLoader::calculateGlobalBoundingBox(std::vector<BoundingBox> const& boundingBoxes)
{
	BoundingBox globalBBox;

	if (boundingBoxes.empty()) {
		globalBBox.min = glm::vec3(0.0f);
		globalBBox.max = glm::vec3(0.0f);
		return globalBBox;
	}

	globalBBox.min = glm::vec3(std::numeric_limits<float>::max());
	globalBBox.max = glm::vec3(std::numeric_limits<float>::lowest());

	for (auto const& bbox : boundingBoxes) {
		globalBBox.min.x = std::min(globalBBox.min.x, bbox.min.x);
		globalBBox.min.y = std::min(globalBBox.min.y, bbox.min.y);
		globalBBox.min.z = std::min(globalBBox.min.z, bbox.min.z);

		globalBBox.max.x = std::max(globalBBox.max.x, bbox.max.x);
		globalBBox.max.y = std::max(globalBBox.max.y, bbox.max.y);
		globalBBox.max.z = std::max(globalBBox.max.z, bbox.max.z);
	}

	return globalBBox;
}