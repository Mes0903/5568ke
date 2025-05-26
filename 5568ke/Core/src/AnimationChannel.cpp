#define GLM_ENABLE_EXPERIMENTAL

#include "AnimationChannel.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <tiny_gltf.h>

void AnimationChannel::loadChannelData(tinygltf::Model const& model, tinygltf::Animation const& anim, tinygltf::AnimationChannel const& channel)
{
	std::cout << "[AnimationChannel INFO] AnimationChannel::loadChannelData - Starting" << std::endl;

	targetNode = channel.target_node;
	std::cout << "[AnimationChannel INFO] Target node: " << targetNode << std::endl;

	// Validate sampler index
	if (channel.sampler < 0 || static_cast<std::size_t>(channel.sampler) >= anim.samplers.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid sampler index: " << channel.sampler << std::endl;
		throw std::runtime_error("Invalid sampler index");
	}

	// Get timing data (input)
	tinygltf::AnimationSampler const& sampler = anim.samplers.at(channel.sampler);

	// Validate input accessor
	if (sampler.input < 0 || static_cast<std::size_t>(sampler.input) >= model.accessors.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid input accessor index: " << sampler.input << std::endl;
		throw std::runtime_error("Invalid input accessor index");
	}

	std::cout << "[AnimationChannel INFO] Getting input accessor: " << sampler.input << std::endl;
	tinygltf::Accessor const& inputAccessor = model.accessors.at(sampler.input);

	// Validate buffer view
	if (inputAccessor.bufferView < 0 || static_cast<std::size_t>(inputAccessor.bufferView) >= model.bufferViews.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid input bufferView index: " << inputAccessor.bufferView << std::endl;
		throw std::runtime_error("Invalid input bufferView index");
	}

	std::cout << "[AnimationChannel INFO] Getting input buffer view: " << inputAccessor.bufferView << std::endl;
	tinygltf::BufferView const& inputBufferView = model.bufferViews.at(inputAccessor.bufferView);

	// Validate buffer
	if (inputBufferView.buffer < 0 || static_cast<std::size_t>(inputBufferView.buffer) >= model.buffers.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid input buffer index: " << inputBufferView.buffer << std::endl;
		throw std::runtime_error("Invalid input buffer index");
	}

	std::cout << "[AnimationChannel INFO] Getting input buffer: " << inputBufferView.buffer << std::endl;
	tinygltf::Buffer const& inputBuffer = model.buffers.at(inputBufferView.buffer);

	// Check if the buffer has enough data
	size_t dataOffset = inputBufferView.byteOffset + inputAccessor.byteOffset;
	if (dataOffset >= inputBuffer.data.size()) {
		std::cout << "[AnimationChannel ERROR] Input buffer offset out of bounds: " << dataOffset << " >= " << inputBuffer.data.size() << std::endl;
		throw std::runtime_error("Input buffer offset out of bounds");
	}

	// Check buffer size
	size_t requiredSize = dataOffset + (inputAccessor.count * sizeof(float));
	if (requiredSize > inputBuffer.data.size()) {
		std::cout << "[AnimationChannel ERROR] Input buffer too small: need " << requiredSize << " but have " << inputBuffer.data.size() << std::endl;
		throw std::runtime_error("Input buffer too small");
	}

	std::cout << "[AnimationChannel INFO] Accessor count: " << inputAccessor.count << ", byteOffset: " << inputAccessor.byteOffset
						<< ", bufferView byteOffset: " << inputBufferView.byteOffset << std::endl;

	// Resize timing array
	timings_.resize(inputAccessor.count);
	std::cout << "[AnimationChannel INFO] Copying timing data, count: " << inputAccessor.count << ", byte length: " << inputBufferView.byteLength << std::endl;

	// Safer copying method with explicit range checking
	unsigned char const* src = inputBuffer.data.data() + dataOffset;
	float* dst = timings_.data();
	for (size_t i = 0; i < inputAccessor.count; i++) {
		if (dataOffset + i * sizeof(float) + sizeof(float) <= inputBuffer.data.size()) {
			memcpy(&dst[i], src + i * sizeof(float), sizeof(float));
		}
		else {
			std::cout << "[AnimationChannel ERROR] Input buffer access out of bounds at index " << i << std::endl;
			throw std::runtime_error("Input buffer access out of bounds");
		}
	}

	std::cout << "[AnimationChannel INFO] Timing data copied successfully" << std::endl;

	// Check and print first few timing values
	if (!timings_.empty()) {
		std::cout << "[AnimationChannel INFO] First " << std::min(size_t(5), timings_.size()) << " timing values:";
		for (size_t i = 0; i < std::min(size_t(5), timings_.size()); i++) {
			std::cout << " " << timings_[i];
		}
		std::cout << std::endl;
	}

	// Set interpolation type
	if (sampler.interpolation == "STEP") {
		interpolationType_ = InterpolationType::STEP;
		std::cout << "[AnimationChannel INFO] Interpolation type: STEP" << std::endl;
	}
	else if (sampler.interpolation == "LINEAR") {
		interpolationType_ = InterpolationType::LINEAR;
		std::cout << "[AnimationChannel INFO] Interpolation type: LINEAR" << std::endl;
	}
	else if (sampler.interpolation == "CUBICSPLINE") {
		interpolationType_ = InterpolationType::CUBICSPLINE;
		std::cout << "[AnimationChannel INFO] Interpolation type: CUBICSPLINE" << std::endl;
	}
	else {
		std::cout << "[AnimationChannel INFO] Unknown interpolation type: " << sampler.interpolation << ", defaulting to LINEAR" << std::endl;
		interpolationType_ = InterpolationType::LINEAR;
	}

	// Validate output accessor
	if (sampler.output < 0 || static_cast<std::size_t>(sampler.output) >= model.accessors.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid output accessor index: " << sampler.output << std::endl;
		throw std::runtime_error("Invalid output accessor index");
	}

	// Get output data
	std::cout << "[AnimationChannel INFO] Getting output accessor: " << sampler.output << std::endl;
	tinygltf::Accessor const& outputAccessor = model.accessors.at(sampler.output);

	// Validate output buffer view
	if (outputAccessor.bufferView < 0 || static_cast<std::size_t>(outputAccessor.bufferView) >= model.bufferViews.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid output bufferView index: " << outputAccessor.bufferView << std::endl;
		throw std::runtime_error("Invalid output bufferView index");
	}

	std::cout << "[AnimationChannel INFO] Getting output buffer view: " << outputAccessor.bufferView << std::endl;
	tinygltf::BufferView const& outputBufferView = model.bufferViews.at(outputAccessor.bufferView);

	// Validate output buffer
	if (outputBufferView.buffer < 0 || static_cast<std::size_t>(outputBufferView.buffer) >= model.buffers.size()) {
		std::cout << "[AnimationChannel ERROR] Invalid output buffer index: " << outputBufferView.buffer << std::endl;
		throw std::runtime_error("Invalid output buffer index");
	}

	std::cout << "[AnimationChannel INFO] Getting output buffer: " << outputBufferView.buffer << std::endl;
	tinygltf::Buffer const& outputBuffer = model.buffers.at(outputBufferView.buffer);

	std::cout << "[AnimationChannel INFO] Output accessor count: " << outputAccessor.count << ", byteOffset: " << outputAccessor.byteOffset
						<< ", bufferView byteOffset: " << outputBufferView.byteOffset << ", componentType: " << outputAccessor.componentType
						<< ", type: " << outputAccessor.type << std::endl;

	// Check buffer sizes and offsets
	size_t outputDataOffset = outputBufferView.byteOffset + outputAccessor.byteOffset;
	if (outputDataOffset >= outputBuffer.data.size()) {
		std::cout << "[AnimationChannel ERROR] Output buffer offset out of bounds: " << outputDataOffset << " >= " << outputBuffer.data.size() << std::endl;
		throw std::runtime_error("Output buffer offset out of bounds");
	}

	// Get element size based on component type and accessor type
	size_t componentSize;
	if (outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		componentSize = sizeof(float);
	}
	else if (outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
		componentSize = sizeof(double);
	}
	else {
		std::cout << "[AnimationChannel ERROR] Unsupported component type: " << outputAccessor.componentType << std::endl;
		throw std::runtime_error("Unsupported component type");
	}

	size_t elementSize;
	if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
		elementSize = componentSize * 3;
	}
	else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
		elementSize = componentSize * 4;
	}
	else {
		std::cout << "[AnimationChannel ERROR] Unsupported accessor type: " << outputAccessor.type << std::endl;
		throw std::runtime_error("Unsupported accessor type");
	}

	size_t outputRequiredSize = outputDataOffset + (outputAccessor.count * elementSize);
	if (outputRequiredSize > outputBuffer.data.size()) {
		std::cout << "[AnimationChannel ERROR] Output buffer too small: need " << outputRequiredSize << " but have " << outputBuffer.data.size() << std::endl;
		throw std::runtime_error("Output buffer too small");
	}

	// Determine the target path
	if (channel.target_path == "rotation") {
		targetPath = TargetPath::ROTATION;
		std::cout << "[AnimationChannel INFO] Target path: ROTATION" << std::endl;

		// Manually load quaternion data with proper size checks
		rotations_.resize(outputAccessor.count);

		unsigned char const* outputSrc = outputBuffer.data.data() + outputDataOffset;
		for (size_t i = 0; i < outputAccessor.count; i++) {
			if (outputDataOffset + i * elementSize + elementSize <= outputBuffer.data.size()) {
				float const* quatData = reinterpret_cast<float const*>(outputSrc + i * elementSize);
				// Note: In glTF, quaternions are stored as [x, y, z, w]
				rotations_[i] = glm::quat(quatData[3], quatData[0], quatData[1], quatData[2]);
			}
			else {
				std::cout << "[AnimationChannel ERROR] Output buffer access out of bounds at index " << i << std::endl;
				throw std::runtime_error("Output buffer access out of bounds");
			}
		}

		std::cout << "[AnimationChannel INFO] Rotation data loaded successfully, size: " << rotations_.size() << std::endl;
		if (!rotations_.empty()) {
			std::cout << "[AnimationChannel INFO] First quaternion: [" << rotations_[0].x << ", " << rotations_[0].y << ", " << rotations_[0].z << ", "
								<< rotations_[0].w << "]" << std::endl;
		}
	}
	else if (channel.target_path == "translation") {
		targetPath = TargetPath::TRANSLATION;
		std::cout << "[AnimationChannel INFO] Target path: TRANSLATION" << std::endl;

		// Manually load translation data with proper size checks
		translations_.resize(outputAccessor.count);

		unsigned char const* outputSrc = outputBuffer.data.data() + outputDataOffset;
		for (size_t i = 0; i < outputAccessor.count; i++) {
			if (outputDataOffset + i * elementSize + elementSize <= outputBuffer.data.size()) {
				float const* vecData = reinterpret_cast<float const*>(outputSrc + i * elementSize);
				translations_[i] = glm::vec3(vecData[0], vecData[1], vecData[2]);
			}
			else {
				std::cout << "[AnimationChannel ERROR] Output buffer access out of bounds at index " << i << std::endl;
				throw std::runtime_error("Output buffer access out of bounds");
			}
		}

		std::cout << "[AnimationChannel INFO] Translation data loaded successfully, size: " << translations_.size() << std::endl;
		if (!translations_.empty()) {
			std::cout << "[AnimationChannel INFO] First translation: [" << translations_[0].x << ", " << translations_[0].y << ", " << translations_[0].z << "]"
								<< std::endl;
		}
	}
	else if (channel.target_path == "scale") {
		targetPath = TargetPath::SCALE;
		std::cout << "[AnimationChannel INFO] Target path: SCALE" << std::endl;

		// Manually load scale data with proper size checks
		scalings_.resize(outputAccessor.count);

		unsigned char const* outputSrc = outputBuffer.data.data() + outputDataOffset;
		for (size_t i = 0; i < outputAccessor.count; i++) {
			if (outputDataOffset + i * elementSize + elementSize <= outputBuffer.data.size()) {
				float const* vecData = reinterpret_cast<float const*>(outputSrc + i * elementSize);
				scalings_[i] = glm::vec3(vecData[0], vecData[1], vecData[2]);
			}
			else {
				std::cout << "[AnimationChannel ERROR] Output buffer access out of bounds at index " << i << std::endl;
				throw std::runtime_error("Output buffer access out of bounds");
			}
		}

		std::cout << "[AnimationChannel INFO] Scale data loaded successfully, size: " << scalings_.size() << std::endl;
		if (!scalings_.empty()) {
			std::cout << "[AnimationChannel INFO] First scale: [" << scalings_[0].x << ", " << scalings_[0].y << ", " << scalings_[0].z << "]" << std::endl;
		}
	}
	else {
		std::cout << "[AnimationChannel ERROR] Unknown target path: " << channel.target_path << std::endl;
		throw std::runtime_error("Unknown target path: " + channel.target_path);
	}

	std::cout << "[AnimationChannel INFO] AnimationChannel::loadChannelData - Completed successfully" << std::endl;
}

float AnimationChannel::getMaxTime() const
{
	if (timings_.empty()) {
		return 0.0f;
	}
	return timings_.back();
}

glm::vec3 AnimationChannel::getScaling(float time) const
{
	if (scalings_.empty()) {
		return glm::vec3(1.0f);
	}

	// Handle edge cases
	if (time <= timings_.front()) {
		return scalings_.front();
	}
	if (time >= timings_.back()) {
		return scalings_.back();
	}

	// Find indices for surrounding keyframes
	size_t nextIdx = 0;
	while (nextIdx < timings_.size() && timings_[nextIdx] < time) {
		nextIdx++;
	}
	size_t prevIdx = nextIdx - 1;

	// Handle special case when indices are the same
	if (prevIdx == nextIdx) {
		return scalings_[prevIdx];
	}

	// Interpolate based on interpolation type
	glm::vec3 result(1.0f);

	switch (interpolationType_) {
	case InterpolationType::STEP:
		result = scalings_[prevIdx];
		break;

	case InterpolationType::LINEAR: {
		float t = (time - timings_[prevIdx]) / (timings_[nextIdx] - timings_[prevIdx]);
		result = glm::mix(scalings_[prevIdx], scalings_[nextIdx], t);
		break;
	}

	case InterpolationType::CUBICSPLINE: {
		// For cubicspline, each keyframe has 3 values: in-tangent, point, out-tangent
		float dt = timings_[nextIdx] - timings_[prevIdx];
		float t = (time - timings_[prevIdx]) / dt;
		float t2 = t * t;
		float t3 = t2 * t;

		size_t prevValueIdx = prevIdx * 3 + 1;			// Point value
		size_t prevOutTangentIdx = prevIdx * 3 + 2; // Out tangent
		size_t nextInTangentIdx = nextIdx * 3;			// In tangent
		size_t nextValueIdx = nextIdx * 3 + 1;			// Point value

		glm::vec3 p0 = scalings_[prevValueIdx];
		glm::vec3 m0 = dt * scalings_[prevOutTangentIdx];
		glm::vec3 p1 = scalings_[nextValueIdx];
		glm::vec3 m1 = dt * scalings_[nextInTangentIdx];

		// Cubic Hermite spline formula
		result = (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 + (t3 - 2.0f * t2 + t) * m0 + (-2.0f * t3 + 3.0f * t2) * p1 + (t3 - t2) * m1;
		break;
	}
	}

	return result;
}

glm::vec3 AnimationChannel::getTranslation(float time) const
{
	if (translations_.empty()) {
		return glm::vec3(0.0f);
	}

	// Handle edge cases
	if (time <= timings_.front()) {
		return translations_.front();
	}
	if (time >= timings_.back()) {
		return translations_.back();
	}

	// Find indices for surrounding keyframes
	size_t nextIdx = 0;
	while (nextIdx < timings_.size() && timings_[nextIdx] < time) {
		nextIdx++;
	}
	size_t prevIdx = nextIdx - 1;

	// Handle special case when indices are the same
	if (prevIdx == nextIdx) {
		return translations_[prevIdx];
	}

	// Interpolate based on interpolation type
	glm::vec3 result(0.0f);

	switch (interpolationType_) {
	case InterpolationType::STEP:
		result = translations_[prevIdx];
		break;

	case InterpolationType::LINEAR: {
		float t = (time - timings_[prevIdx]) / (timings_[nextIdx] - timings_[prevIdx]);
		result = glm::mix(translations_[prevIdx], translations_[nextIdx], t);
		break;
	}

	case InterpolationType::CUBICSPLINE: {
		// For cubicspline, each keyframe has 3 values: in-tangent, point, out-tangent
		float dt = timings_[nextIdx] - timings_[prevIdx];
		float t = (time - timings_[prevIdx]) / dt;
		float t2 = t * t;
		float t3 = t2 * t;

		size_t prevValueIdx = prevIdx * 3 + 1;			// Point value
		size_t prevOutTangentIdx = prevIdx * 3 + 2; // Out tangent
		size_t nextInTangentIdx = nextIdx * 3;			// In tangent
		size_t nextValueIdx = nextIdx * 3 + 1;			// Point value

		glm::vec3 p0 = translations_[prevValueIdx];
		glm::vec3 m0 = dt * translations_[prevOutTangentIdx];
		glm::vec3 p1 = translations_[nextValueIdx];
		glm::vec3 m1 = dt * translations_[nextInTangentIdx];

		// Cubic Hermite spline formula
		result = (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 + (t3 - 2.0f * t2 + t) * m0 + (-2.0f * t3 + 3.0f * t2) * p1 + (t3 - t2) * m1;
		break;
	}
	}

	return result;
}

glm::quat AnimationChannel::getRotation(float time) const
{
	if (rotations_.empty()) {
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}

	// Handle edge cases
	if (time <= timings_.front()) {
		return rotations_.front();
	}
	if (time >= timings_.back()) {
		return rotations_.back();
	}

	// Find indices for surrounding keyframes
	size_t nextIdx = 0;
	while (nextIdx < timings_.size() && timings_[nextIdx] < time) {
		nextIdx++;
	}
	size_t prevIdx = nextIdx - 1;

	// Handle special case when indices are the same
	if (prevIdx == nextIdx) {
		return rotations_[prevIdx];
	}

	// Interpolate based on interpolation type
	glm::quat result(1.0f, 0.0f, 0.0f, 0.0f);

	switch (interpolationType_) {
	case InterpolationType::STEP:
		result = rotations_[prevIdx];
		break;

	case InterpolationType::LINEAR: {
		float t = (time - timings_[prevIdx]) / (timings_[nextIdx] - timings_[prevIdx]);
		result = glm::slerp(rotations_[prevIdx], rotations_[nextIdx], t);
		break;
	}

	case InterpolationType::CUBICSPLINE: {
		// For cubicspline, each keyframe has 3 values: in-tangent, point, out-tangent
		float dt = timings_[nextIdx] - timings_[prevIdx];
		float t = (time - timings_[prevIdx]) / dt;
		float t2 = t * t;
		float t3 = t2 * t;

		size_t prevValueIdx = prevIdx * 3 + 1;			// Point value
		size_t prevOutTangentIdx = prevIdx * 3 + 2; // Out tangent
		size_t nextInTangentIdx = nextIdx * 3;			// In tangent
		size_t nextValueIdx = nextIdx * 3 + 1;			// Point value

		glm::quat p0 = rotations_[prevValueIdx];
		glm::quat m0 = dt * rotations_[prevOutTangentIdx];
		glm::quat p1 = rotations_[nextValueIdx];
		glm::quat m1 = dt * rotations_[nextInTangentIdx];

		// Cubic Hermite spline formula for quaternions
		// This is an approximation, as quaternion interpolation is more complex
		result = (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 + (t3 - 2.0f * t2 + t) * m0 + (-2.0f * t3 + 3.0f * t2) * p1 + (t3 - t2) * m1;

		// Normalize the resulting quaternion
		result = glm::normalize(result);
		break;
	}
	}

	return result;
}
