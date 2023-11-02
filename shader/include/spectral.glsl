vec3 cie_colour_match[81] = {
    vec3(0.0014f,0.0000f,0.0065f), vec3(0.0022f,0.0001f,0.0105f), vec3(0.0042f,0.0001f,0.0201f),
    vec3(0.0076f,0.0002f,0.0362f), vec3(0.0143f,0.0004f,0.0679f), vec3(0.0232f,0.0006f,0.1102f),
    vec3(0.0435f,0.0012f,0.2074f), vec3(0.0776f,0.0022f,0.3713f), vec3(0.1344f,0.0040f,0.6456f),
    vec3(0.2148f,0.0073f,1.0391f), vec3(0.2839f,0.0116f,1.3856f), vec3(0.3285f,0.0168f,1.6230f),
    vec3(0.3483f,0.0230f,1.7471f), vec3(0.3481f,0.0298f,1.7826f), vec3(0.3362f,0.0380f,1.7721f),
    vec3(0.3187f,0.0480f,1.7441f), vec3(0.2908f,0.0600f,1.6692f), vec3(0.2511f,0.0739f,1.5281f),
    vec3(0.1954f,0.0910f,1.2876f), vec3(0.1421f,0.1126f,1.0419f), vec3(0.0956f,0.1390f,0.8130f),
    vec3(0.0580f,0.1693f,0.6162f), vec3(0.0320f,0.2080f,0.4652f), vec3(0.0147f,0.2586f,0.3533f),
    vec3(0.0049f,0.3230f,0.2720f), vec3(0.0024f,0.4073f,0.2123f), vec3(0.0093f,0.5030f,0.1582f),
    vec3(0.0291f,0.6082f,0.1117f), vec3(0.0633f,0.7100f,0.0782f), vec3(0.1096f,0.7932f,0.0573f),
    vec3(0.1655f,0.8620f,0.0422f), vec3(0.2257f,0.9149f,0.0298f), vec3(0.2904f,0.9540f,0.0203f),
    vec3(0.3597f,0.9803f,0.0134f), vec3(0.4334f,0.9950f,0.0087f), vec3(0.5121f,1.0000f,0.0057f),
    vec3(0.5945f,0.9950f,0.0039f), vec3(0.6784f,0.9786f,0.0027f), vec3(0.7621f,0.9520f,0.0021f),
    vec3(0.8425f,0.9154f,0.0018f), vec3(0.9163f,0.8700f,0.0017f), vec3(0.9786f,0.8163f,0.0014f),
    vec3(1.0263f,0.7570f,0.0011f), vec3(1.0567f,0.6949f,0.0010f), vec3(1.0622f,0.6310f,0.0008f),
    vec3(1.0456f,0.5668f,0.0006f), vec3(1.0026f,0.5030f,0.0003f), vec3(0.9384f,0.4412f,0.0002f),
    vec3(0.8544f,0.3810f,0.0002f), vec3(0.7514f,0.3210f,0.0001f), vec3(0.6424f,0.2650f,0.0000f),
    vec3(0.5419f,0.2170f,0.0000f), vec3(0.4479f,0.1750f,0.0000f), vec3(0.3608f,0.1382f,0.0000f),
    vec3(0.2835f,0.1070f,0.0000f), vec3(0.2187f,0.0816f,0.0000f), vec3(0.1649f,0.0610f,0.0000f),
    vec3(0.1212f,0.0446f,0.0000f), vec3(0.0874f,0.0320f,0.0000f), vec3(0.0636f,0.0232f,0.0000f),
    vec3(0.0468f,0.0170f,0.0000f), vec3(0.0329f,0.0119f,0.0000f), vec3(0.0227f,0.0082f,0.0000f),
    vec3(0.0158f,0.0057f,0.0000f), vec3(0.0114f,0.0041f,0.0000f), vec3(0.0081f,0.0029f,0.0000f),
    vec3(0.0058f,0.0021f,0.0000f), vec3(0.0041f,0.0015f,0.0000f), vec3(0.0029f,0.0010f,0.0000f),
    vec3(0.0020f,0.0007f,0.0000f), vec3(0.0014f,0.0005f,0.0000f), vec3(0.0010f,0.0004f,0.0000f),
    vec3(0.0007f,0.0002f,0.0000f), vec3(0.0005f,0.0002f,0.0000f), vec3(0.0003f,0.0001f,0.0000f),
    vec3(0.0002f,0.0001f,0.0000f), vec3(0.0002f,0.0001f,0.0000f), vec3(0.0001f,0.0000f,0.0000f),
    vec3(0.0001f,0.0000f,0.0000f), vec3(0.0001f,0.0000f,0.0000f), vec3(0.0000f,0.0000f,0.0000f)
};

vec3 xyz_to_rgb(in vec3 xyz)
{
    // https://www.fourmilab.ch/documents/specrend/
    const float xr = 0.7355f;
    const float yr = 0.2645f;
    const float zr = 1 - xr - yr;
    const float xg = 0.2658f;
    const float yg = 0.7243f;
    const float zg = 1 - xg - yg;
    const float xb = 0.1669f;
    const float yb = 0.0085f;
    const float zb = 1 - xb - yb;
    const float xw = 0.33333333f;
    const float yw = 0.33333333f;
    const float zw = 0.33333333f;

    // xyz -> rgb matrix, before scaling to white
    const float rx = (yg * zb) - (yb * zg);
    const float ry = (xb * zg) - (xg * zb);
    const float rz = (xg * yb) - (xb * yg);
    const float gx = (yb * zr) - (yr * zb);
    const float gy = (xr * zb) - (xb * zr);
    const float gz = (xb * yr) - (xr * yb);
    const float bx = (yr * zg) - (yg * zr);
    const float by = (xg * zr) - (xr * zg);
    const float bz = (xr * yg) - (xg * yr);

    // White scaling factors. Dividing by yw scales the white luminance to unity, as conventional
    const float rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
    const float gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
    const float bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

    // xyz -> rgb matrix, correctly scaled to white
    const float scaled_rx = rx / rw;
    const float scaled_ry = ry / rw;
    const float scaled_rz = rz / rw;
    const float scaled_gx = gx / gw;
    const float scaled_gy = gy / gw;
    const float scaled_gz = gz / gw;
    const float scaled_bx = bx / bw;
    const float scaled_by = by / bw;
    const float scaled_bz = bz / bw;

    // rgb of the desired point
    vec3 rgb;
    rgb.r = (scaled_rx * xyz.x) + (scaled_ry * xyz.y) + (scaled_rz * xyz.z);
    rgb.g = (scaled_gx * xyz.x) + (scaled_gy * xyz.y) + (scaled_gz * xyz.z);
    rgb.b = (scaled_bx * xyz.x) + (scaled_by * xyz.y) + (scaled_bz * xyz.z);

    // Amount of white needed is w = - min(0, r, g, b)
    float w = (0.0f < rgb.r) ? 0.0f : rgb.r;
    w = (w < rgb.g) ? w : rgb.g;
    w = (w < rgb.b) ? w : rgb.b;
    w = -w;

    // Add just enough white to make r, g, b all positive
    rgb.r += w;
    rgb.g += w;
    rgb.b += w;

    float greatest = max(rgb.r, max(rgb.g, rgb.b));
    if (greatest > 0.0f) {
        rgb.r /= greatest;
        rgb.g /= greatest;
        rgb.b /= greatest;
    }
    return rgb;
}

vec4 wavelength_to_rgba(uint wavelength)
{
    // divide by average response
    return vec4(xyz_to_rgb(cie_colour_match[(wavelength - 380) / 5]) * vec3(1.0f / 0.542192f, 1.0f / 0.275452f, 1.0f / 0.347872f), 1.0f);
}

uint get_random_wavelength(float random)
{
    return 380 + uint(pcg_random_state() * 80.999) * 5;
}

// uniform samples of wavelength -> every sample has the same probability
float get_inv_wavelength_probability()
{
    return 80.0;
}
