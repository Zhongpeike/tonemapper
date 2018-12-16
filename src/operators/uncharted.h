/*
    src/uncharted.h -- Uncharted tonemapping operator

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <tonemap.h>

class UnchartedOperator : public TonemapOperator {
public:
	UnchartedOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["A"] = Parameter(0.22f, 0.f, 1.f, "A", "Shoulder strength curve parameter");
		parameters["B"] = Parameter(0.3f, 0.f, 1.f, "B", "Linear strength curve parameter");
		parameters["C"] = Parameter(0.1f, 0.f, 1.f, "C", "Linear angle curve parameter");
		parameters["D"] = Parameter(0.2f, 0.f, 1.f, "D", "Toe strength curve parameter");
		parameters["E"] = Parameter(0.01f, 0.f, 1.f, "E", "Toe numerator curve parameter");
		parameters["F"] = Parameter(0.3f, 0.f, 1.f, "F", "Toe denominator curve parameter");
		parameters["W"] = Parameter(11.2f, 0.f, 20.f, "W", "White point\nMinimal value that is mapped to 1.");

		name = "Uncharted (Hable)";
		description = "Uncharted Mapping\n\nBy John Hable from the \"Filmic Tonemapping for Real-time Rendering\" Siggraph 2010 Course by Haarm-Pieter Duiker.";

		shader->init(
			"Uncharted",

			"#version 330\n"
			"in vec2 position;\n"
			"out vec2 uv;\n"
			"void main() {\n"
			"    gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);\n"
			"    uv = vec2(position.x, 1-position.y);\n"
			"}",

			"#version 330\n"
			"uniform sampler2D source;\n"
			"uniform float exposure;\n"
			"uniform float gamma;\n"
			"uniform float A;\n"
			"uniform float B;\n"
			"uniform float C;\n"
			"uniform float D;\n"
			"uniform float E;\n"
			"uniform float F;\n"
			"uniform float W;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"vec4 gammaCorrect(vec4 color) {\n"
			"	 return pow(color, vec4(1.0/gamma));\n"
			"}\n"
			"\n"
			"vec4 tonemap(vec4 x) {\n"
			"	 return ((x * (A*x + C*B) + D*E) / (x * (A*x+B) + D*F)) - E/F;\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float exposureBias = 2.0;\n"
			"	 vec4 curr = tonemap(exposureBias * color);\n"
			"	 vec4 whiteScale = 1.0 / tonemap(vec4(W));\n"
			"	 color = curr * whiteScale;\n"
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float A = parameters.at("A").value;
		float B = parameters.at("B").value;
		float C = parameters.at("C").value;
		float D = parameters.at("D").value;
		float E = parameters.at("E").value;
		float F = parameters.at("F").value;
		float W = parameters.at("W").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				Color3f c = Color3f(map(color.r(), exposure, A, B, C, D, E, F, W),
									map(color.g(), exposure, A, B, C, D, E, F, W),
									map(color.b(), exposure, A, B, C, D, E, F, W));
				c = c.clampedValue();
				c = c.gammaCorrect(gamma);
				dst[0] = (uint8_t) (255.f * c.r());
				dst[1] = (uint8_t) (255.f * c.g());
				dst[2] = (uint8_t) (255.f * c.b());
				dst += 3;
				*progress += delta;
			}
		}
	}

	float graph(float value) const override {
		float gamma = parameters.at("Gamma").value;
		float A = parameters.at("A").value;
		float B = parameters.at("B").value;
		float C = parameters.at("C").value;
		float D = parameters.at("D").value;
		float E = parameters.at("E").value;
		float F = parameters.at("F").value;
		float W = parameters.at("W").value;

		value = map(value, 1.f, A, B, C, D, E, F, W);
		value = clamp(value, 0.f, 1.f);
		value = std::pow(value, 1.f / gamma);
		return value;
	}

protected:
	float map(float v, float exposure, float A, float B, float C, float D, float E, float F, float W) const {
		float value = exposure * v;
		float exposureBias = 2.f;
		value = mapAux(exposureBias * value, A, B, C, D, E, F);
		float whiteScale = 1.f / mapAux(W, A, B, C, D, E, F);
		value = value * whiteScale;
		return value;
	}

protected:
	float mapAux(float x, float A, float B, float C, float D, float E, float F) const {
		return ((x * (A*x + C*B) + D*E) / (x * (A*x+B) + D*F)) - E/F;
	}
};