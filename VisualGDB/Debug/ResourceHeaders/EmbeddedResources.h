#pragma once

extern void *_binary_VisualGDB_Debug_R6581_VS_Display_hex_start, *_binary_VisualGDB_Debug_R6581_VS_Display_hex_end, *_binary_VisualGDB_Debug_R6581_VS_Display_hex_size;
#ifdef __cplusplus
#ifndef CUSTOM_EMBEDDED_RESOURCE_CLASS
template <void **_Start, void **_End, void **_Size> class EmbeddedResource
{
public:
	void *data() { return _Start; }
	void *end() { return _End; }
	unsigned size()  { return (unsigned)_Size; }
};
#endif

namespace EmbeddedResources
{
	static EmbeddedResource<&_binary_VisualGDB_Debug_R6581_VS_Display_hex_start, &_binary_VisualGDB_Debug_R6581_VS_Display_hex_end, &_binary_VisualGDB_Debug_R6581_VS_Display_hex_size> VisualGDB_Debug_R6581_VS_Display_hex;
}
#endif
