#define GLM_ENABLE_EXPERIMENTAL

#include "GltfLoader.hpp"

#include <chrono>
#include <cmath>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "AnimationClip.hpp"
#include "BlinnPhongMaterial.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Node.hpp"
#include "Primitive.hpp"
#include "Vertex.hpp"

std::shared_ptr<Model> GltfLoader::loadModel(std::string const& path) { return loadGltf_(path, MaterialType::BlinnPhong); }

std::shared_ptr<Model> GltfLoader::loadGltf_(std::string const& path, MaterialType type)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	// Enable verbose debug output
	loader.SetStoreOriginalJSONForExtrasAndExtensions(true);

	// Determine file type (GLTF or GLB) and load accordingly
	bool ret;
	if (path.find(".glb") != std::string::npos)
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
	else
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);

	// Handle loading errors
	if (!warn.empty()) {
		std::cout << "[GltfLoader INFO] GLTF warning: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cout << "[GltfLoader INFO] GLTF error: " << err << std::endl;
		return nullptr;
	}

	if (!ret) {
		std::cout << "[GltfLoader INFO] Failed to load GLTF file: " << path << std::endl;
		return nullptr;
	}

	// Create and populate model
	std::shared_ptr<Model> model = std::make_shared<Model>();
	model->meshNodeIndices.resize(gltfModel.meshes.size(), -1);

	std::cout << "[GltfLoader INFO] GLTF file has:\n"
						<< gltfModel.accessors.size() << " accessors\n"
						<< gltfModel.animations.size() << " animations\n"
						<< gltfModel.buffers.size() << " buffers\n"
						<< gltfModel.bufferViews.size() << " bufferViews\n"
						<< gltfModel.materials.size() << " materials\n"
						<< gltfModel.meshes.size() << " meshes\n"
						<< gltfModel.nodes.size() << " nodes\n"
						<< gltfModel.textures.size() << " textures\n"
						<< gltfModel.images.size() << " images\n"
						<< gltfModel.skins.size() << " skins\n"
						<< gltfModel.samplers.size() << " samplers\n"
						<< gltfModel.cameras.size() << " cameras\n"
						<< gltfModel.scenes.size() << " scenes\n"
						<< gltfModel.lights.size() << " lights\n"
						<< gltfModel.audioEmitters.size() << " audioEmitters\n"
						<< gltfModel.audioSources.size() << " audioSources\n";

	// Process all meshes in the GLTF file
	for (tinygltf::Mesh& mesh : gltfModel.meshes) {
		Mesh outMesh;
		processMesh_(gltfModel, mesh, outMesh, type);

		// Setup OpenGL buffers and VAO
		outMesh.setup();

		// Calculate bounding box
		BoundingBox bbox = BBoxUtil::getMeshBBox(outMesh);
		model->boundingBoxes.push_back(bbox);

		// Add mesh to model
		model->meshes.push_back(std::move(outMesh));
	}

	// Map each mesh to its node
	for (size_t i = 0; i < gltfModel.nodes.size(); i++) {
		auto const& node = gltfModel.nodes[i];
		if (node.mesh >= 0 && static_cast<std::size_t>(node.mesh) < model->meshNodeIndices.size()) {
			model->meshNodeIndices[node.mesh] = i;
			std::cout << "[GltfLoader] Node " << i << " references mesh " << node.mesh << std::endl;
		}
	}

	// Load node hierarchy and skin data if available
	loadNodeHierarchy_(model, gltfModel);

	if (!gltfModel.skins.empty()) {
		std::cout << "[GltfLoader INFO] Find skin data, starting to load skin data." << std::endl;
		loadSkinData_(model, gltfModel);
	}

	// Load animations if available
	if (!gltfModel.animations.empty()) {
		loadAnimations_(model, gltfModel);
		model->updateLocalMatrices();
		std::cout << "[GltfLoader INFO] Loaded " << model->animations.size() << " animation clips" << std::endl;
	}

	if (model->rootNode) {
		// Calculate all node matrices starting from the root
		NodeUtil::updateNodeTreeMatricesRecursive(model->rootNode, glm::mat4(1.0f));
		std::cout << "[GltfLoader] Node matrices calculated for static transforms" << std::endl;
	}

	// Calculate global bounding box and store on the model
	if (!model->boundingBoxes.empty()) {
		BBoxUtil::updateLocalBBox(*model);

		// Print global bounding box info
		std::cout << "[GltfLoader INFO] Model global bounding box: min(" << model->localSpaceBBox.min.x << ", " << model->localSpaceBBox.min.y << ", "
							<< model->localSpaceBBox.min.z << "), max(" << model->localSpaceBBox.max.x << ", " << model->localSpaceBBox.max.y << ", "
							<< model->localSpaceBBox.max.z << ")" << std::endl;
	}

	return model;
}

Texture* GltfLoader::loadTexture_(tinygltf::Model const& model, int textureIndex, TextureType type)
{
	if (textureIndex < 0 || static_cast<std::size_t>(textureIndex) >= model.textures.size())
		return nullptr;

	auto* texture = new Texture();
	texture->type = type;

	tinygltf::Texture const& gltfTexture = model.textures[textureIndex];
	if (gltfTexture.source < 0 || static_cast<std::size_t>(gltfTexture.source) >= model.images.size()) {
		delete texture;
		return nullptr;
	}

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

	std::cout << "[GltfLoader INFO] Loading texture: " << image.uri << " (" << image.width << "x" << image.height << ", components: " << image.component << ")"
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

Material* GltfLoader::createMaterial_(tinygltf::Model const& model, tinygltf::Primitive const& primitive, MaterialType type)
{
	// Make sure we can access BlinnPhongMaterial class
	if (type == MaterialType::BlinnPhong) {
		// Create the material
		BlinnPhongMaterial* material = new BlinnPhongMaterial();

		// Check if material exists in the model
		if (primitive.material >= 0 && static_cast<std::size_t>(primitive.material) < model.materials.size()) {
			tinygltf::Material const& mat = model.materials[primitive.material];

			// Set base color if available
			if (mat.pbrMetallicRoughness.baseColorFactor.size() >= 3) {
				material->albedo =
						glm::vec3(mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);
				std::cout << "[GltfLoader INFO] Material albedo: " << material->albedo.x << ", " << material->albedo.y << ", " << material->albedo.z << std::endl;
			}

			// Set roughness as inverse of shininess
			if (mat.pbrMetallicRoughness.roughnessFactor >= 0) {
				// Convert roughness to shininess (inverse relationship)
				float roughness = mat.pbrMetallicRoughness.roughnessFactor;
				material->shininess = std::max(2.0f, 128.0f * (1.0f - roughness));
				std::cout << "[GltfLoader INFO] Material shininess: " << material->shininess << std::endl;
			}

			// Load diffuse texture if available
			if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
				material->diffuseMap = loadTexture_(model, mat.pbrMetallicRoughness.baseColorTexture.index, TextureType::Diffuse);
				std::cout << "[GltfLoader INFO] Loaded diffuse texture" << std::endl;
			}

			// Check for additional textures that could be used for overlay
			if (mat.normalTexture.index >= 0) {
				material->overlayMap = loadTexture_(model, mat.normalTexture.index, TextureType::Normal);
				std::cout << "[GltfLoader INFO] Loaded normal/overlay texture" << std::endl;
			}
		}

		return material;
	}

	// Default fallback - create a basic BlinnPhongMaterial
	return new BlinnPhongMaterial();
}

/**
 * @brief Convert one glTF mesh into 'Mesh' structure, filling in:
 * - 'outMesh.vertices': raw vertex array
 * - 'outMesh.indices': index buffer (EBO)
 * - 'outMesh.primitives': per-material draw calls (Primitive)
 */
void GltfLoader::processMesh_(tinygltf::Model const& model, tinygltf::Mesh const& mesh, Mesh& outMesh, MaterialType materialType)
{
	// Iterate over every primitive in the glTF mesh
	for (tinygltf::Primitive const& primitive : mesh.primitives) {
		Primitive outPrimitive;
		outPrimitive.indexOffset = outMesh.indices.size(); // The first index of this primitive inside the big, concatenated index buffer we are building.
																											 // The renderer will add this offset when it calls 'glDrawElements' later.
		outPrimitive.doubleSided = (primitive.material >= 0 && model.materials[primitive.material].doubleSided);

		// Get position attribute
		if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
			int posAccessorIndex = primitive.attributes.at("POSITION");
			tinygltf::Accessor const& accessor = model.accessors[posAccessorIndex];					 // How many vertices and where in a BufferView.
			tinygltf::BufferView const& bufferView = model.bufferViews[accessor.bufferView]; // Slice of the raw binary Buffer (could be interleaved).
			tinygltf::Buffer const& buffer = model.buffers[bufferView.buffer];							 // The actual 'std::vector<uint8_t> data'.

			size_t const byteOffset = accessor.byteOffset + bufferView.byteOffset;
			size_t const byteStride = accessor.ByteStride(bufferView);
			size_t const count = accessor.count;

			// 'vertexStart' remembers where this primitive’s vertices begin in the big vertex array.
			size_t vertexStart = outMesh.vertices.size();
			outMesh.vertices.resize(vertexStart + count);

			// Populate positions
			for (size_t i = 0; i < count; i++) {
				float const* pos = reinterpret_cast<float const*>(&buffer.data[byteOffset + i * byteStride]);
				outMesh.vertices[vertexStart + i].position = glm::vec3(pos[0], pos[1], pos[2]);
			}

			// Get normal attribute if exists
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				int normalAccessorIndex = primitive.attributes.at("NORMAL");
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
				int texcoordAccessorIndex = primitive.attributes.at("TEXCOORD_0");
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

			// Process JOINTS and WEIGHTS for vertex skinning
			if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end() && primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {

				int jointsAccessorIndex = primitive.attributes.at("JOINTS_0");
				int weightsAccessorIndex = primitive.attributes.at("WEIGHTS_0");

				tinygltf::Accessor const& jointsAccessor = model.accessors[jointsAccessorIndex];
				tinygltf::BufferView const& jointsBufferView = model.bufferViews[jointsAccessor.bufferView];
				tinygltf::Buffer const& jointsBuffer = model.buffers[jointsBufferView.buffer];

				tinygltf::Accessor const& weightsAccessor = model.accessors[weightsAccessorIndex];
				tinygltf::BufferView const& weightsBufferView = model.bufferViews[weightsAccessor.bufferView];
				tinygltf::Buffer const& weightsBuffer = model.buffers[weightsBufferView.buffer];

				// Debug output
				std::cout << "[GltfLoader] Processing skinning data: " << count << " vertices, joint type: " << jointsAccessor.componentType << std::endl;

				// Process each vertex's skinning data
				for (size_t i = 0; i < count; i++) {
					if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
						uint16_t const* joints = reinterpret_cast<uint16_t const*>(
								&jointsBuffer.data[jointsBufferView.byteOffset + jointsAccessor.byteOffset + i * jointsAccessor.ByteStride(jointsBufferView)]);

						for (int j = 0; j < 4; j++) {
							outMesh.vertices[vertexStart + i].boneIds[j] = static_cast<int>(joints[j]);
						}
					}
					else if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
						uint8_t const* joints = reinterpret_cast<uint8_t const*>(
								&jointsBuffer.data[jointsBufferView.byteOffset + jointsAccessor.byteOffset + i * jointsAccessor.ByteStride(jointsBufferView)]);

						for (int j = 0; j < 4; j++) {
							outMesh.vertices[vertexStart + i].boneIds[j] = static_cast<int>(joints[j]);
						}
					}

					// Get weights
					float const* weights = reinterpret_cast<float const*>(
							&weightsBuffer.data[weightsBufferView.byteOffset + weightsAccessor.byteOffset + i * weightsAccessor.ByteStride(weightsBufferView)]);

					// Store weights
					outMesh.vertices[vertexStart + i].boneWeights = glm::vec4(weights[0], weights[1], weights[2], weights[3]);

					// Ensure weights sum to 1
					float sum = weights[0] + weights[1] + weights[2] + weights[3];
					if (sum > 0 && std::abs(sum - 1.0f) > 0.01f) {
						outMesh.vertices[vertexStart + i].boneWeights /= sum;
					}
				}

				std::cout << "[GltfLoader] Successfully loaded skinning data" << std::endl;
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
			outPrimitive.material = createMaterial_(model, primitive, materialType);
			outMesh.primitives.push_back(outPrimitive);
		}
	}
}

void GltfLoader::loadAnimations_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel)
{
	std::cout << "[GltfLoader INFO] Starting to load animations. Count: " << gltfModel.animations.size() << std::endl;

	// Clear any existing animations
	model->animations.clear();

	for (size_t animIndex = 0; animIndex < gltfModel.animations.size(); animIndex++) {
		tinygltf::Animation const& anim = gltfModel.animations[animIndex];

		// Create animation clip
		std::string clipName = anim.name.empty() ? "Animation_" + std::to_string(animIndex) : anim.name;
		std::cout << "[GltfLoader INFO] Creating animation clip: " << clipName << std::endl;

		auto clip = std::make_shared<AnimationClip>(clipName);

		std::cout << "[GltfLoader INFO] Loading animation '" << clipName << "' with " << anim.channels.size() << " channels" << std::endl;

		// Process animation channels
		for (size_t channelIndex = 0; channelIndex < anim.channels.size(); channelIndex++) {
			auto const& channel = anim.channels[channelIndex];
			std::cout << "[GltfLoader INFO] Processing channel " << channelIndex << ", target_node: " << channel.target_node
								<< ", target_path: " << channel.target_path << ", sampler: " << channel.sampler << std::endl;

			// Skip channels targeting nodes we don't have
			if (channel.target_node < 0 || static_cast<std::size_t>(channel.target_node) >= model->nodes.size()) {
				std::cout << "[GltfLoader ERROR] Invalid target_node index: " << channel.target_node << std::endl;
				continue;
			}

			// Skip invalid samplers
			if (channel.sampler < 0 || static_cast<std::size_t>(channel.sampler) >= anim.samplers.size()) {
				std::cout << "[GltfLoader ERROR] Invalid sampler index: " << channel.sampler << std::endl;
				continue;
			}

			try {
				clip->addChannel(gltfModel, anim, channel);
				std::cout << "[GltfLoader INFO] Successfully added channel " << channelIndex << std::endl;
			} catch (std::exception const& e) {
				std::cout << "[GltfLoader ERROR] Exception while adding channel: " << e.what() << std::endl;
			} catch (...) {
				std::cout << "[GltfLoader ERROR] Unknown exception while adding channel" << std::endl;
			}
		}

		// Only add the clip if it has valid channels
		if (clip->getDuration() > 0) {
			std::cout << "[GltfLoader INFO] Animation '" << clipName << "' has duration: " << clip->getDuration() << std::endl;
			model->animations.push_back(clip);
		}
		else {
			std::cout << "[GltfLoader INFO] Skipping animation '" << clipName << "' with zero duration" << std::endl;
		}
	}

	std::cout << "[GltfLoader INFO] Finished loading all animations. Total: " << model->animations.size() << std::endl;
}

void GltfLoader::loadNodeHierarchy_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel)
{
	std::cout << "[GltfLoader INFO] Starting to load node hierarchy" << std::endl;

	// Resize nodes vector to fit all nodes in the model
	model->nodes.resize(gltfModel.nodes.size());
	std::cout << "[GltfLoader INFO] Resized nodes vector to " << gltfModel.nodes.size() << " elements" << std::endl;

	// Find root node (usually the first node in the default scene)
	int rootNodeIndex = 0;
	if (!gltfModel.scenes.empty() && !gltfModel.scenes[0].nodes.empty()) {
		rootNodeIndex = gltfModel.scenes[0].nodes[0];
	}

	std::cout << "[GltfLoader INFO] Model has " << gltfModel.nodes.size() << " nodes, root node is " << rootNodeIndex << std::endl;

	// Validate root node index
	if (rootNodeIndex < 0 || static_cast<std::size_t>(rootNodeIndex) >= gltfModel.nodes.size()) {
		std::cout << "[GltfLoader ERROR] Invalid root node index: " << rootNodeIndex << std::endl;
		return;
	}

	try {
		// Create the root node
		std::cout << "[GltfLoader INFO] Creating root node with index " << rootNodeIndex << std::endl;
		model->rootNode = NodeUtil::createRoot(rootNodeIndex);

		if (!model->rootNode) {
			std::cout << "[GltfLoader ERROR] Failed to create root node" << std::endl;
			return;
		}

		model->nodes[rootNodeIndex] = model->rootNode;

		// Process the entire node hierarchy starting from the root
		std::cout << "[GltfLoader INFO] Processing node hierarchy starting from root" << std::endl;
		processNodeTreeRecursive_(model, gltfModel, rootNodeIndex, glm::mat4(1.0f));

		std::cout << "[GltfLoader INFO] Node hierarchy loaded successfully" << std::endl;
	} catch (std::exception const& e) {
		std::cout << "[GltfLoader ERROR] Exception while loading node hierarchy: " << e.what() << std::endl;
	} catch (...) {
		std::cout << "[GltfLoader ERROR] Unknown exception while loading node hierarchy" << std::endl;
	}
}

void GltfLoader::processNodeTreeRecursive_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel, int nodeIndex, glm::mat4 const& parentMatrix)
{
	std::cout << "[GltfLoader INFO] Processing node " << nodeIndex << std::endl;

	// Validate node index
	if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= gltfModel.nodes.size()) {
		std::cout << "[GltfLoader ERROR] Invalid node index: " << nodeIndex << std::endl;
		return;
	}

	// Make sure this node has been created first
	if (!model->nodes[nodeIndex]) {
		// Create the node if it doesn't exist
		model->nodes[nodeIndex] = std::make_shared<Node>(nodeIndex);
		std::cout << "[GltfLoader INFO] Created missing node for index " << nodeIndex << std::endl;
	}

	tinygltf::Node const& node = gltfModel.nodes[nodeIndex];
	std::shared_ptr<Node> currentNode = model->nodes[nodeIndex];

	// Set node name
	std::string nodeName = node.name.empty() ? "Node_" + std::to_string(nodeIndex) : node.name;
	currentNode->nodeName = nodeName;
	std::cout << "[GltfLoader INFO] Node name: " << nodeName << std::endl;

	// Set transformation components, the matrix must be an 4x4 homogeneous matrix
	if (!node.matrix.empty() && node.matrix.size() == 16) {
		std::cout << "[GltfLoader INFO] Detected matrix in node " << nodeIndex << std::endl;
		glm::mat4 nodeMatrix = glm::make_mat4(node.matrix.data());

		// Decompose the matrix into TRS components
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;

		if (glm::decompose(nodeMatrix, scale, rotation, translation, skew, perspective)) {
			currentNode->translation = translation;
			currentNode->rotation = rotation;
			currentNode->scale = scale;

			std::cout << "[GltfLoader INFO] Node " << nodeIndex << " matrix decomposed to: "
								<< "T(" << translation.x << "," << translation.y << "," << translation.z << ") "
								<< "R(" << rotation.x << "," << rotation.y << "," << rotation.z << ") "
								<< "S(" << scale.x << "," << scale.y << "," << scale.z << ")" << std::endl;
		}
		else {
			std::cout << "[GltfLoader WARNING] Failed to decompose matrix for node " << nodeIndex << std::endl;
			// Set identity transformation as fallback
			currentNode->translation = glm::vec3(0.0f);
			currentNode->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			currentNode->scale = glm::vec3(1.0f);
		}
	}

	// Deal with TRS transformation, normally glTF does not allow the matrix to coexist with the TRS, but we still check the transformation here
	if (!node.translation.empty()) {
		if (node.translation.size() >= 3) {
			currentNode->translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
			std::cout << "[GltfLoader INFO] Node translation: " << node.translation[0] << ", " << node.translation[1] << ", " << node.translation[2] << std::endl;
		}
		else {
			std::cout << "[GltfLoader WARNING] Node " << nodeIndex << " has incomplete translation data" << std::endl;
		}
	}

	if (!node.rotation.empty()) {
		if (node.rotation.size() >= 4) {
			currentNode->rotation =
					glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]); // w component comes last in the array but first in glm::quat
			std::cout << "[GltfLoader INFO] Node rotation: " << node.rotation[0] << ", " << node.rotation[1] << ", " << node.rotation[2] << ", " << node.rotation[3]
								<< std::endl;
		}
		else {
			std::cout << "[GltfLoader WARNING] Node " << nodeIndex << " has incomplete rotation data" << std::endl;
		}
	}

	if (!node.scale.empty()) {
		if (node.scale.size() >= 3) {
			currentNode->scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
			std::cout << "[GltfLoader INFO] Node scale: " << node.scale[0] << ", " << node.scale[1] << ", " << node.scale[2] << std::endl;
		}
		else {
			std::cout << "[GltfLoader WARNING] Node " << nodeIndex << " has incomplete scale data" << std::endl;
		}
	}

	try {
		// Calculate matrices
		currentNode->updateLocalTRSMatrix();
		currentNode->updateNodeMatrix(parentMatrix);

		// Process child nodes
		std::cout << "[GltfLoader INFO] Node " << nodeIndex << " has " << node.children.size() << " children" << std::endl;
		for (size_t i = 0; i < node.children.size(); i++) {
			int childIndex = node.children[i];

			// Skip child nodes that are skins to avoid confusion in the hierarchy
			if (childIndex >= 0 && static_cast<std::size_t>(childIndex) < gltfModel.nodes.size() && gltfModel.nodes[childIndex].skin != -1) {
				std::cout << "[GltfLoader INFO] Skipping skin child node " << childIndex << std::endl;
				continue;
			}

			// Validate child index
			if (childIndex < 0 || static_cast<std::size_t>(childIndex) >= gltfModel.nodes.size()) {
				std::cout << "[GltfLoader ERROR] Invalid child node index: " << childIndex << std::endl;
				continue;
			}

			std::cout << "[GltfLoader INFO] Processing child node " << childIndex << std::endl;

			// Create child node if it doesn't exist
			if (!model->nodes[childIndex]) {
				model->nodes[childIndex] = std::make_shared<Node>(childIndex);
				std::cout << "[GltfLoader INFO] Created new node for child " << childIndex << std::endl;
			}

			// Get the child node from the array
			std::shared_ptr<Node> childNode = model->nodes[childIndex];

			// Directly add to current node's children list
			currentNode->children.push_back(childNode);

			// Recursively process child node
			processNodeTreeRecursive_(model, gltfModel, childIndex, currentNode->getNodeMatrix());
		}
	} catch (std::exception const& e) {
		std::cout << "[GltfLoader ERROR] Exception while processing node " << nodeIndex << ": " << e.what() << std::endl;
	} catch (...) {
		std::cout << "[GltfLoader ERROR] Unknown exception while processing node " << nodeIndex << std::endl;
	}
}

void GltfLoader::loadSkinData_(std::shared_ptr<Model> model, tinygltf::Model const& gltfModel)
{
	// Process only the first skin for simplicity
	tinygltf::Skin const& skin = gltfModel.skins[0];

	// Load inverse bind matrices
	// Note: skinnedPosition = jointMatrix * inverseBindMatrix * vertexPosition;
	if (skin.inverseBindMatrices >= 0) {
		tinygltf::Accessor const& accessor = gltfModel.accessors[skin.inverseBindMatrices];
		tinygltf::BufferView const& bufferView = gltfModel.bufferViews[accessor.bufferView];
		tinygltf::Buffer const& buffer = gltfModel.buffers[bufferView.buffer];

		size_t numMatrices = accessor.count;
		model->inverseBindMatrices.resize(numMatrices);

		float const* data = reinterpret_cast<float const*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < numMatrices; i++) {
			model->inverseBindMatrices[i] = glm::make_mat4(data + i * 16);
		}

		std::cout << "[GltfLoader INFO] Loaded " << numMatrices << " inverse bind matrices" << std::endl;
	}

	// Create joint mapping
	model->nodeToJointMapping.resize(gltfModel.nodes.size(), -1);
	for (size_t i = 0; i < skin.joints.size(); i++) {
		int nodeIndex = skin.joints[i];
		model->nodeToJointMapping[nodeIndex] = static_cast<int>(i);
	}

	// Initialize joint matrices with identity matrices
	model->jointMatrices.resize(skin.joints.size(), glm::mat4(1.0f));

	// Load vertex joint and weight data
	if (!gltfModel.meshes.empty() && !gltfModel.meshes[0].primitives.empty()) {
		tinygltf::Primitive const& primitive = gltfModel.meshes[0].primitives[0];

		auto jointsIt = primitive.attributes.find("JOINTS_0");	 // which bones this vertex is bound to (index)
		auto weightsIt = primitive.attributes.find("WEIGHTS_0"); // the influence (weight) of the corresponding bones

		if (jointsIt != primitive.attributes.end() && weightsIt != primitive.attributes.end()) {
			int jointsAccessorIndex = jointsIt->second;
			int weightsAccessorIndex = weightsIt->second;

			tinygltf::Accessor const& jointsAccessor = gltfModel.accessors[jointsAccessorIndex];
			tinygltf::BufferView const& jointsBufferView = gltfModel.bufferViews[jointsAccessor.bufferView];
			tinygltf::Buffer const& jointsBuffer = gltfModel.buffers[jointsBufferView.buffer];

			tinygltf::Accessor const& weightsAccessor = gltfModel.accessors[weightsAccessorIndex];
			tinygltf::BufferView const& weightsBufferView = gltfModel.bufferViews[weightsAccessor.bufferView];
			tinygltf::Buffer const& weightsBuffer = gltfModel.buffers[weightsBufferView.buffer];

			size_t vertexCount = jointsAccessor.count;
			model->vertexJoints.resize(vertexCount);

			if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				uint16_t const* jointData = reinterpret_cast<uint16_t const*>(&jointsBuffer.data[jointsBufferView.byteOffset + jointsAccessor.byteOffset]);
				float const* weightData = reinterpret_cast<float const*>(&weightsBuffer.data[weightsBufferView.byteOffset + weightsAccessor.byteOffset]);

				for (size_t i = 0; i < vertexCount; i++) {
					for (int j = 0; j < 4; j++) {
						int jointIndex = jointData[i * 4 + j];
						float weight = weightData[i * 4 + j];

						if (weight > 0.0f) {
							model->vertexJoints[i].push_back(std::make_pair(jointIndex, weight));
						}
					}
				}
			}
			else if (jointsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				uint8_t const* jointData = reinterpret_cast<uint8_t const*>(&jointsBuffer.data[jointsBufferView.byteOffset + jointsAccessor.byteOffset]);
				float const* weightData = reinterpret_cast<float const*>(&weightsBuffer.data[weightsBufferView.byteOffset + weightsAccessor.byteOffset]);

				for (size_t i = 0; i < vertexCount; i++) {
					for (int j = 0; j < 4; j++) {
						int jointIndex = jointData[i * 4 + j];
						float weight = weightData[i * 4 + j];

						if (weight > 0.0f) {
							model->vertexJoints[i].push_back(std::make_pair(jointIndex, weight));
						}
					}
				}
			}

			std::cout << "[GltfLoader INFO] Loaded joint weights for " << vertexCount << " vertices" << std::endl;
		}
	}
}
