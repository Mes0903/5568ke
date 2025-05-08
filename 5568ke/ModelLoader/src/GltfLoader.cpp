#include "GltfLoader.hpp"

#include "include_5568ke.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <queue>

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

// Implement loadGltf method with animation support
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
	model->filePath = path;
	model->name = std::filesystem::path(path).stem().string();

	std::cout << "[GltfLoader]  GLTF file has " << gltfModel.meshes.size() << " meshes, " << gltfModel.textures.size() << " textures, "
						<< gltfModel.materials.size() << " materials, " << gltfModel.animations.size() << " animations, " << gltfModel.skins.size() << " skins"
						<< std::endl;

	// Initialize skeleton if model has skins
	if (!gltfModel.skins.empty()) {
		model->hasAnimations = true;
		model->skeleton.initBoneMatrices();
		loadSkeleton(gltfModel, model);
	}

	// Process all meshes in the GLTF file
	for (size_t i = 0; i < gltfModel.meshes.size(); i++) {
		tinygltf::Mesh& mesh = gltfModel.meshes[i];
		Mesh outMesh;
		processMesh(gltfModel, mesh, outMesh, type);

		// Apply bone weights to vertices if model has animations
		if (model->hasAnimations) {
			applyVertexBoneData(gltfModel, i, outMesh, model->skeleton);
		}

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

	// Load animations if available
	if (model->hasAnimations && !gltfModel.animations.empty()) {
		loadAnimations(gltfModel, model);

		// Initialize animation player
		model->animationPlayer.initialize(model);

		// Start playing the first animation by default
		if (!model->animations.empty()) {
			model->animationPlayer.setAnimation(0);
			model->animationPlayer.play();
		}
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

void GltfLoader::loadSkeleton(tinygltf::Model& model, Model* outModel)
{
	if (model.skins.empty()) {
		std::cout << "[GltfLoader] No skins found in model" << std::endl;
		return;
	}

	// Process the first skin
	int skinIndex = 0;
	processSkin(model, skinIndex, outModel->skeleton);
}

void GltfLoader::processSkin(tinygltf::Model& model, int skinIndex, Skeleton& skeleton)
{
	if (skinIndex < 0 || skinIndex >= static_cast<int>(model.skins.size())) {
		std::cerr << "[GltfLoader] Invalid skin index: " << skinIndex << std::endl;
		return;
	}

	tinygltf::Skin const& skin = model.skins[skinIndex];

	std::cout << "[GltfLoader] Processing skin: " << skin.name << std::endl;
	std::cout << "[GltfLoader] Skin has " << skin.joints.size() << " joints" << std::endl;

	// Process inverse bind matrices if available
	std::vector<glm::mat4> inverseBindMatrices;
	if (skin.inverseBindMatrices >= 0) {
		tinygltf::Accessor const& accessor = model.accessors[skin.inverseBindMatrices];
		tinygltf::BufferView const& bufferView = model.bufferViews[accessor.bufferView];
		tinygltf::Buffer const& buffer = model.buffers[bufferView.buffer];

		float const* matrices = reinterpret_cast<float const*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);

		for (size_t i = 0; i < skin.joints.size(); i++) {
			// Each matrix is 16 floats (4x4)
			float const* mat = matrices + (i * 16);
			inverseBindMatrices.push_back(aiMatrix4x4ToGlm(mat));
		}
	}
	else {
		// If no inverse bind matrices, use identity matrices
		inverseBindMatrices.resize(skin.joints.size(), glm::mat4(1.0f));
	}

	// Reserve space for bones
	skeleton.bones.reserve(skin.joints.size());

	// Create bones for each joint
	for (size_t i = 0; i < skin.joints.size(); i++) {
		int nodeIndex = skin.joints[i];
		tinygltf::Node const& node = model.nodes[nodeIndex];

		Bone bone;
		bone.name = node.name.empty() ? "joint_" + std::to_string(i) : node.name;
		bone.id = static_cast<int>(i);
		bone.offsetMatrix = inverseBindMatrices[i];

		// Store the bone
		skeleton.bones.push_back(bone);
		skeleton.boneNameToIndex[bone.name] = bone.id;
	}

	// Set bone count
	skeleton.boneCount = static_cast<int>(skeleton.bones.size());

	std::cout << "[GltfLoader] Loaded " << skeleton.boneCount << " bones" << std::endl;
}

void GltfLoader::applyVertexBoneData(tinygltf::Model& model, int meshIndex, Mesh& outMesh, Skeleton& skeleton)
{
	if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) {
		std::cerr << "[GltfLoader] Invalid mesh index: " << meshIndex << std::endl;
		return;
	}

	tinygltf::Mesh const& mesh = model.meshes[meshIndex];

	// Iterate over primitives
	for (size_t primIndex = 0; primIndex < mesh.primitives.size(); primIndex++) {
		tinygltf::Primitive const& primitive = mesh.primitives[primIndex];

		// Check for joints and weights attributes
		if (primitive.attributes.find("JOINTS_0") == primitive.attributes.end() || primitive.attributes.find("WEIGHTS_0") == primitive.attributes.end()) {
			continue;
		}

		// Get joints
		int jointsAccessorIndex = primitive.attributes.at("JOINTS_0");
		tinygltf::Accessor const& jointsAccessor = model.accessors[jointsAccessorIndex];
		tinygltf::BufferView const& jointsBufferView = model.bufferViews[jointsAccessor.bufferView];
		tinygltf::Buffer const& jointsBuffer = model.buffers[jointsBufferView.buffer];

		unsigned char const* jointsData = &jointsBuffer.data[jointsAccessor.byteOffset + jointsBufferView.byteOffset];

		// Get weights
		int weightsAccessorIndex = primitive.attributes.at("WEIGHTS_0");
		tinygltf::Accessor const& weightsAccessor = model.accessors[weightsAccessorIndex];
		tinygltf::BufferView const& weightsBufferView = model.bufferViews[weightsAccessor.bufferView];
		tinygltf::Buffer const& weightsBuffer = model.buffers[weightsBufferView.buffer];

		float const* weightsData = reinterpret_cast<float const*>(&weightsBuffer.data[weightsAccessor.byteOffset + weightsBufferView.byteOffset]);

		// Get vertex offset for this primitive
		size_t vertexStart = 0;
		if (primIndex > 0) {
			// Calculate vertex offset based on previous primitives
			for (size_t i = 0; i < primIndex; i++) {
				// This is a simplification - assumes each primitive has the same number of vertices
				tinygltf::Primitive const& prevPrim = mesh.primitives[i];
				if (prevPrim.attributes.find("POSITION") != prevPrim.attributes.end()) {
					int posAccessorIndex = prevPrim.attributes.at("POSITION");
					tinygltf::Accessor const& posAccessor = model.accessors[posAccessorIndex];
					vertexStart += posAccessor.count;
				}
			}
		}

		// Get vertex count for this primitive
		size_t vertexCount = 0;
		if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
			int posAccessorIndex = primitive.attributes.at("POSITION");
			tinygltf::Accessor const& posAccessor = model.accessors[posAccessorIndex];
			vertexCount = posAccessor.count;
		}

		// Apply bone data to vertices
		for (size_t i = 0; i < vertexCount; i++) {
			if (vertexStart + i >= outMesh.vertices.size()) {
				continue;
			}

			// Get joint indices and weights
			glm::ivec4 jointIndices;
			glm::vec4 jointWeights;

			// Read joint indices based on component type
			if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				uint8_t const* data = reinterpret_cast<uint8_t const*>(jointsData) + (i * 4);
				jointIndices = glm::ivec4(data[0], data[1], data[2], data[3]);
			}
			else if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				uint16_t const* data = reinterpret_cast<uint16_t const*>(jointsData) + (i * 4);
				jointIndices = glm::ivec4(data[0], data[1], data[2], data[3]);
			}

			// Read weights
			jointWeights = glm::vec4(weightsData[i * 4], weightsData[i * 4 + 1], weightsData[i * 4 + 2], weightsData[i * 4 + 3]);

			// Apply joint indices and weights to vertex
			for (int j = 0; j < 4; j++) {
				if (jointWeights[j] > 0.0f) {
					outMesh.vertices[vertexStart + i].boneData.addBoneData(jointIndices[j], jointWeights[j]);
				}
			}

			// Normalize weights to ensure they sum to 1
			outMesh.vertices[vertexStart + i].boneData.normalize();
		}
	}

	std::cout << "[GltfLoader] Applied bone data to mesh vertices" << std::endl;
}

void GltfLoader::loadAnimations(tinygltf::Model& model, Model* outModel)
{
	if (model.animations.empty()) {
		std::cout << "[GltfLoader] No animations found in model" << std::endl;
		return;
	}

	std::cout << "[GltfLoader] Loading " << model.animations.size() << " animations" << std::endl;

	// Process each animation
	for (size_t i = 0; i < model.animations.size(); i++) {
		processAnimation(model, i, outModel);
	}
}

void GltfLoader::processAnimation(tinygltf::Model& model, int animIndex, Model* outModel)
{
	if (animIndex < 0 || animIndex >= static_cast<int>(model.animations.size())) {
		std::cerr << "[GltfLoader] Invalid animation index: " << animIndex << std::endl;
		return;
	}

	tinygltf::Animation const& gltfAnim = model.animations[animIndex];

	// Create a new animation
	Animation animation;
	animation.name = gltfAnim.name.empty() ? "animation_" + std::to_string(animIndex) : gltfAnim.name;

	std::cout << "[GltfLoader] Processing animation: " << animation.name << std::endl;

	// Set default ticks per second if not specified
	animation.ticksPerSecond = 25.0f;

	// Process animation channels
	for (size_t i = 0; i < gltfAnim.channels.size(); i++) {
		tinygltf::AnimationChannel const& channel = gltfAnim.channels[i];

		// Skip if target node is invalid
		if (channel.target_node < 0 || channel.target_node >= static_cast<int>(model.nodes.size())) {
			continue;
		}

		// Get the node name
		tinygltf::Node const& targetNode = model.nodes[channel.target_node];
		std::string nodeName = targetNode.name.empty() ? "node_" + std::to_string(channel.target_node) : targetNode.name;

		// Find corresponding bone
		int boneIndex = outModel->skeleton.getBoneIndex(nodeName);
		if (boneIndex < 0) {
			continue;
		}

		// Extract keyframes for this channel
		Bone& bone = outModel->skeleton.bones[boneIndex];
		extractKeyframes(model, gltfAnim, i, bone);

		// Update animation duration to the max time of all keyframes
		if (!bone.positions.empty()) {
			animation.duration = std::max(animation.duration, bone.positions.back().timeStamp);
		}
		if (!bone.rotations.empty()) {
			animation.duration = std::max(animation.duration, bone.rotations.back().timeStamp);
		}
		if (!bone.scales.empty()) {
			animation.duration = std::max(animation.duration, bone.scales.back().timeStamp);
		}
	}

	std::cout << "[GltfLoader] Animation duration: " << animation.duration << " ticks" << std::endl;

	// Create animation hierarchy
	animation.rootNode = new SkeletonNode();
	animation.rootNode->name = "root";
	animation.rootNode->transformation = glm::mat4(1.0f);
	animation.nodeMap["root"] = animation.rootNode;

	// Process scene nodes to build hierarchy
	if (model.scenes.size() > 0) {
		tinygltf::Scene const& scene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];

		for (size_t i = 0; i < scene.nodes.size(); i++) {
			SkeletonNode* node = new SkeletonNode();
			processNode(model, scene.nodes[i], animation.rootNode, node, animation.nodeMap);
			animation.rootNode->children.push_back(node);
		}
	}

	// Add animation to model
	outModel->animations.push_back(animation);
}

void GltfLoader::processNode(tinygltf::Model& model, int nodeIndex, SkeletonNode* parent, SkeletonNode* outNode,
														 std::unordered_map<std::string, SkeletonNode*>& nodeMap)
{
	if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size())) {
		return;
	}

	tinygltf::Node const& node = model.nodes[nodeIndex];

	// Set node properties
	outNode->name = node.name.empty() ? "node_" + std::to_string(nodeIndex) : node.name;

	// Try to find corresponding bone
	// You'll need to implement this based on your skeleton structure
	outNode->boneIndex = -1; // Default to no bone

	// Calculate transformation matrix
	glm::mat4 transform = glm::mat4(1.0f);

	// Apply translation if present
	if (node.translation.size() == 3) {
		transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
	}

	// Apply rotation if present
	if (node.rotation.size() == 4) {
		glm::quat q = glm::quat(node.rotation[3], // w
														node.rotation[0], // x
														node.rotation[1], // y
														node.rotation[2]	// z
		);
		transform = transform * glm::toMat4(q);
	}

	// Apply scale if present
	if (node.scale.size() == 3) {
		transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
	}

	// Or use provided matrix if available
	if (node.matrix.size() == 16) {
		transform = glm::mat4(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3], node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
													node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11], node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
	}

	outNode->transformation = transform;

	// Add to node map
	nodeMap[outNode->name] = outNode;

	// Process children
	for (size_t i = 0; i < node.children.size(); i++) {
		SkeletonNode* childNode = new SkeletonNode();
		processNode(model, node.children[i], outNode, childNode, nodeMap);
		outNode->children.push_back(childNode);
	}
}

void GltfLoader::extractKeyframes(tinygltf::Model& model, tinygltf::Animation const& anim, int channelIndex, Bone& bone)
{
	tinygltf::AnimationChannel const& channel = anim.channels[channelIndex];
	tinygltf::AnimationSampler const& sampler = anim.samplers[channel.sampler];

	// Get input accessor (times)
	tinygltf::Accessor const& inputAccessor = model.accessors[sampler.input];
	tinygltf::BufferView const& inputView = model.bufferViews[inputAccessor.bufferView];
	tinygltf::Buffer const& inputBuffer = model.buffers[inputView.buffer];

	float const* times = reinterpret_cast<float const*>(&inputBuffer.data[inputAccessor.byteOffset + inputView.byteOffset]);

	// Get output accessor (values)
	tinygltf::Accessor const& outputAccessor = model.accessors[sampler.output];
	tinygltf::BufferView const& outputView = model.bufferViews[outputAccessor.bufferView];
	tinygltf::Buffer const& outputBuffer = model.buffers[outputView.buffer];

	float const* values = reinterpret_cast<float const*>(&outputBuffer.data[outputAccessor.byteOffset + outputView.byteOffset]);

	// Process keyframes based on target path
	if (channel.target_path == "translation") {
		// Position keyframes
		for (size_t i = 0; i < inputAccessor.count; i++) {
			KeyPosition keyframe;
			keyframe.timeStamp = times[i];
			keyframe.position = glm::vec3(values[i * 3], values[i * 3 + 1], values[i * 3 + 2]);
			bone.positions.push_back(keyframe);
		}
	}
	else if (channel.target_path == "rotation") {
		// Rotation keyframes
		for (size_t i = 0; i < inputAccessor.count; i++) {
			KeyRotation keyframe;
			keyframe.timeStamp = times[i];

			// GLTF quaternions are stored as [x, y, z, w]
			keyframe.orientation = glm::quat(values[i * 4 + 3], // w
																			 values[i * 4],			// x
																			 values[i * 4 + 1], // y
																			 values[i * 4 + 2]	// z
			);

			bone.rotations.push_back(keyframe);
		}
	}
	else if (channel.target_path == "scale") {
		// Scale keyframes
		for (size_t i = 0; i < inputAccessor.count; i++) {
			KeyScale keyframe;
			keyframe.timeStamp = times[i];
			keyframe.scale = glm::vec3(values[i * 3], values[i * 3 + 1], values[i * 3 + 2]);
			bone.scales.push_back(keyframe);
		}
	}
}

// Helper conversion methods
glm::mat4 GltfLoader::aiMatrix4x4ToGlm(float const* mat)
{
	return glm::mat4(mat[0], mat[1], mat[2], mat[3], mat[4], mat[5], mat[6], mat[7], mat[8], mat[9], mat[10], mat[11], mat[12], mat[13], mat[14], mat[15]);
}

glm::quat GltfLoader::aiQuaternionToGlm(float const* q)
{
	return glm::quat(q[3], q[0], q[1], q[2]); // w, x, y, z
}

glm::vec3 GltfLoader::aiVector3DToGlm(float const* vec) { return glm::vec3(vec[0], vec[1], vec[2]); }