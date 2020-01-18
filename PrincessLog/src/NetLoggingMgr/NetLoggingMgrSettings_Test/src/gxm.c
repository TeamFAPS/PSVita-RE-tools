
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/message_dialog.h>
#include <stdlib.h>

#define DISPLAY_WIDTH           960
#define DISPLAY_HEIGHT          544
#define DISPLAY_BUFFER_COUNT        2

static void				*s_contextHost;
static SceUID				s_vdmRingBufUid;
static SceUID				s_vertexRingBufUid;
static SceUID				s_fragmentRingBufUid;
static SceUID				s_fragmentUsseRingBufUid;
static SceGxmContext			*s_context;

#define	DISPLAY_STRIDE			1024
#define	DISPLAY_PENDING_SWAPS		2
#define	DISPLAY_BUFFER_SIZE		((4 * DISPLAY_STRIDE * DISPLAY_HEIGHT + 0xfffffU) & ~0xfffffU)
#define	DISPLAY_ALIGN_WIDTH		((DISPLAY_WIDTH  + SCE_GXM_TILE_SIZEX - 1) & ~(SCE_GXM_TILE_SIZEX - 1))
#define	DISPLAY_ALIGN_HEIGHT		((DISPLAY_HEIGHT + SCE_GXM_TILE_SIZEY - 1) & ~(SCE_GXM_TILE_SIZEY - 1))

#define	SHADER_CLEAR_V_INPUT_PARAM_POSITOIN		"aPosition"
#define	SHADER_FIXEDLIGHT_V_INPUT_PARAM_POSITOIN	"aPosition"
#define	SHADER_FIXEDLIGHT_V_INPUT_PARAM_NORMAL		"aNormal"
#define	SHADER_FIXEDLIGHT_V_UNIFORM_PARAM_WVP		"wvp"
#define	SHADER_FIXEDLIGHT_V_UNIFORM_PARAM_LPOS		"lpos"
#define	SHADER_FIXEDLIGHT_V_UNIFORM_PARAM_EPOS		"epos"
#define	SHADER_FIXEDLIGHT_V_UNIFORM_PARAM_COLOR		"color"

typedef struct {
	float		x;
	float		y;
	float		z;
	float		nx;
	float		ny;
	float		nz;
} VertexV32N32;

typedef struct {
	float		x;
	float		y;
} VertexV32XY;

typedef struct {
	void		*address;
} Display;

// libgxm context
static void				*s_contextHost;
static SceUID				s_vdmRingBufUid;
static SceUID				s_vertexRingBufUid;
static SceUID				s_fragmentRingBufUid;
static SceUID				s_fragmentUsseRingBufUid;
static SceGxmContext			*s_context;

// shader patcher
static SceUID				s_patcherBufUid;
static SceUID				s_patcherCombinedUsseUid;
static SceGxmShaderPatcher		*s_shaderPatcher;

// render target
static SceGxmRenderTarget		*s_renderTarget;

// display buffer variables
static SceUID				s_dispUid[DISPLAY_BUFFER_COUNT];
static void				*s_dispBuf[DISPLAY_BUFFER_COUNT];
static SceGxmSyncObject			*s_dispSync[DISPLAY_BUFFER_COUNT];
static SceGxmColorSurface		s_dispSurface[DISPLAY_BUFFER_COUNT];
static SceUInt32			s_dispFront;
static SceUInt32			s_dispBack;

// depth stencil buffer variables
static SceGxmDepthStencilSurface	s_depthSurface;
static SceUID				s_depthUid;
static void				*s_depthBuf;

// clear shader variables
static SceGxmShaderPatcherId	s_vertexPidCls;
static SceGxmShaderPatcherId	s_fragmentPidCls;
static SceGxmVertexProgram	*s_vertexPgmCls;
static SceGxmFragmentProgram	*s_fragmentPgmCls;

// cube shader variables
static SceGxmShaderPatcherId	s_vertexPid;
static SceGxmShaderPatcherId	s_fragmentPid;
static SceGxmVertexProgram	*s_vertexPgm;
static SceGxmFragmentProgram	*s_fragmentPgm;

// clear vertex variables
static SceUID			s_vertexUidCls;
static SceUID			s_indexUidCls;
static VertexV32XY		*s_vertexBufCls;
static SceUInt16		*s_indexBufCls;

#define	PATCHER_BUFFER_SIZE		(64*1024)
#define	PATCHER_COMBINED_USSE_SIZE	(64*1024)

#define	SCE_UID_INVALID_UID	0xFFFFFFFF

static uint32_t get_attribute(SceGxmShaderPatcherId programId, const char *pName){

	const SceGxmProgram				*pProgram;
	const SceGxmProgramParameter	*pParam;
	int		ret;

	pProgram = sceGxmShaderPatcherGetProgramFromId(programId);

	pParam = sceGxmProgramFindParameterByName(pProgram, pName);
	if (!pParam)
		return 0xffffffff;

	ret = sceGxmProgramParameterGetCategory(pParam);
	if (ret != SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE)
		return 0xffffffff;

	return sceGxmProgramParameterGetResourceIndex(pParam);
}

static void *combined_usse_alloc(uint32_t size, SceUID *uid, unsigned int *vertexUsseOffset, unsigned int *fragmentUsseOffset){
	void	*mem = NULL;
	int		res;

	// align to memblock alignment for LPDDR
	size = (size + 0xfffU) & ~0xfffU;

	res = sceKernelAllocMemBlock("", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (res < 0)
		return NULL;
	*uid = res;

	res = sceKernelGetMemBlockBase(*uid, &mem);
	if (res != 0)
		return NULL;
	res = sceGxmMapVertexUsseMemory(mem, size, vertexUsseOffset);
	if (res != 0)
		return NULL;
	res = sceGxmMapFragmentUsseMemory(mem, size, fragmentUsseOffset);
	if (res != 0)
		return NULL;

	return mem;
}

static void combined_usse_free(SceUID uid){
	void *mem = NULL;
	sceKernelGetMemBlockBase(uid, &mem);
	sceGxmUnmapFragmentUsseMemory(mem);
	sceGxmUnmapVertexUsseMemory(mem);
	sceKernelFreeMemBlock(uid);
}

static void *patcher_host_alloc(void *data, SceSize size){
	return malloc(size);
}

static void patcher_host_free(void *data, void *mem){
	free(mem);
}

static void *fragment_usse_alloc(uint32_t size, SceUID *uid, unsigned int *usseOffset){

	void	*mem = NULL;
	int		res;

	// align to memblock alignment for LPDDR
	size = (size + 0xfffU) & ~0xfffU;

	res = sceKernelAllocMemBlock("", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (res < 0)
		return NULL;
	*uid = res;

	res = sceKernelGetMemBlockBase(*uid, &mem);
	if (res != 0)
		return NULL;
	res = sceGxmMapFragmentUsseMemory(mem, size, usseOffset);
	if (res != 0)
		return NULL;

	return mem;
}

static void fragment_usse_free(SceUID uid){
	void *mem = NULL;
	sceKernelGetMemBlockBase(uid, &mem);
	sceGxmUnmapFragmentUsseMemory(mem);
	sceKernelFreeMemBlock(uid);
}

static void *g_alloc(SceKernelMemBlockType type, SceUInt32 size, SceUInt32 alignment, SceUInt32 attribs, SceUID *uid){

	void	*mem = NULL;
	int		res;

	// CDRAM memblocks must be 256KiB aligned
	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW) {
		if (alignment > 0x40000)
			return NULL;
		size = (size + 0x3ffffU) & ~0x3ffffU;
	}
	// LPDDR memblocks must be 4KiB aligned
	else {
		if (alignment > 0x1000)
			return NULL;
		size = (size + 0xfffU) & ~0xfffU;
	}

	res = sceKernelAllocMemBlock("", type, size, NULL);
	if(res < 0)
		return NULL;
	*uid = res;

	res = sceKernelGetMemBlockBase(*uid, &mem);
	if(res != 0)
		return NULL;
	res = sceGxmMapMemory(mem, size, attribs);
	if(res != 0)
		return NULL;

	return mem;
}

static void g_free(SceUID uid){
	void *mem = NULL;
	sceKernelGetMemBlockBase(uid, &mem);
	sceGxmUnmapMemory(mem);
	sceKernelFreeMemBlock(uid);
}

static void cb_display(const void *data){

	const Display *disp = (const Display *)data;
	SceDisplayFrameBuf framebuf;

	memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
	framebuf.size        = sizeof(SceDisplayFrameBuf);
	framebuf.base        = disp->address;
	framebuf.pitch       = DISPLAY_STRIDE;
	framebuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	framebuf.width       = DISPLAY_WIDTH;
	framebuf.height      = DISPLAY_HEIGHT;
	sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

	sceDisplayWaitVblankStart();
}

extern const SceGxmProgram	_binary_clear_v_gxp_start;
extern const SceGxmProgram	_binary_clear_f_gxp_start;
extern const SceGxmProgram	_binary_fixedlight_v_gxp_start;
extern const SceGxmProgram	_binary_fixedlight_f_gxp_start;

int gxm_initialized = 0;

int gxm_init(void){

	SceGxmInitializeParams		iparam;
	SceGxmContextParams		cparam;
	SceGxmRenderTargetParams	rparam;
	SceGxmShaderPatcherParams	pparam;

	void		*ringbufVdm;
	void		*ringbufVertex;
	void		*ringbufFragment;
	void		*ringbufFragmentUsse;
	SceUInt32	ringbufFragmentUsseOffset;
	void		*patcherBuf;
	SceUInt32	patcherVertexUsseOffset;
	SceUInt32	patcherFragmentUsseOffset;
	void		*patcherCombinedUsse;

	SceGxmVertexAttribute	attr[2];
	SceGxmVertexStream	stream[1];

	int i;

	if(gxm_initialized){
		return 0;
	}

	// start libgxm
	memset(&iparam, 0, sizeof(SceGxmInitializeParams));
	iparam.flags                        = 0;
	iparam.displayQueueMaxPendingCount  = DISPLAY_PENDING_SWAPS;
	iparam.displayQueueCallback         = cb_display;
	iparam.displayQueueCallbackDataSize = sizeof(Display);
	iparam.parameterBufferSize	    = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

	sceGxmInitialize(&iparam);

	// create the context
	s_contextHost = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);

	ringbufVdm = g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, 4,
		SCE_GXM_MEMORY_ATTRIB_READ, &s_vdmRingBufUid);

	ringbufVertex = g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, 4,
		SCE_GXM_MEMORY_ATTRIB_READ, &s_vertexRingBufUid);

	ringbufFragment = g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, 4,
		SCE_GXM_MEMORY_ATTRIB_READ, &s_fragmentRingBufUid);

	ringbufFragmentUsse = fragment_usse_alloc(
		SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
		&s_fragmentUsseRingBufUid, (unsigned int *)&ringbufFragmentUsseOffset);

	memset(&cparam, 0, sizeof(SceGxmContextParams));
	cparam.hostMem                       = s_contextHost;
	cparam.hostMemSize                   = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	cparam.vdmRingBufferMem              = ringbufVdm;
	cparam.vdmRingBufferMemSize          = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
	cparam.vertexRingBufferMem           = ringbufVertex;
	cparam.vertexRingBufferMemSize       = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
	cparam.fragmentRingBufferMem         = ringbufFragment;
	cparam.fragmentRingBufferMemSize     = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
	cparam.fragmentUsseRingBufferMem     = ringbufFragmentUsse;
	cparam.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
	cparam.fragmentUsseRingBufferOffset  = ringbufFragmentUsseOffset;

	s_context = NULL;
	sceGxmCreateContext(&cparam, &s_context);

	// create the render target
	memset(&rparam, 0, sizeof(SceGxmRenderTargetParams));
	rparam.flags                = 0;
	rparam.width                = DISPLAY_WIDTH;
	rparam.height               = DISPLAY_HEIGHT;
	rparam.scenesPerFrame       = 1;
	rparam.multisampleMode      = SCE_GXM_MULTISAMPLE_NONE;
	rparam.multisampleLocations = 0;
	rparam.driverMemBlock       = SCE_UID_INVALID_UID;

	s_renderTarget = NULL;
	sceGxmCreateRenderTarget(&rparam, &s_renderTarget);

	for (i = 0; i<DISPLAY_BUFFER_COUNT; i++) {
		s_dispBuf[i] = g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
			DISPLAY_BUFFER_SIZE,
			SCE_GXM_COLOR_SURFACE_ALIGNMENT,
			SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
			&s_dispUid[i]);

		sceGxmColorSurfaceInit(&s_dispSurface[i],
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			DISPLAY_WIDTH,
			DISPLAY_HEIGHT,
			DISPLAY_STRIDE,
			s_dispBuf[i]);

		sceGxmSyncObjectCreate(&s_dispSync[i]);
	}

	s_dispFront = 0;
	s_dispBack = 0;

	// create depth buffer
	s_depthBuf = g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
			(DISPLAY_ALIGN_WIDTH * DISPLAY_ALIGN_HEIGHT) * 4,
			SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT,
			SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
			&s_depthUid);

	sceGxmDepthStencilSurfaceInit(&s_depthSurface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		DISPLAY_ALIGN_WIDTH,
		s_depthBuf,
		NULL);

	// create a shader patcher
	patcherBuf = g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		PATCHER_BUFFER_SIZE, 4,
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE, &s_patcherBufUid);

	patcherCombinedUsse = combined_usse_alloc(
		PATCHER_COMBINED_USSE_SIZE, &s_patcherCombinedUsseUid,
		(unsigned int *)&patcherVertexUsseOffset, (unsigned int *)&patcherFragmentUsseOffset);

	memset(&pparam, 0, sizeof(SceGxmShaderPatcherParams));
	pparam.userData                  = NULL;
	pparam.hostAllocCallback         = &patcher_host_alloc;
	pparam.hostFreeCallback          = &patcher_host_free;
	pparam.bufferAllocCallback       = NULL;
	pparam.bufferFreeCallback        = NULL;
	pparam.bufferMem                 = patcherBuf;
	pparam.bufferMemSize             = PATCHER_BUFFER_SIZE;
	pparam.vertexUsseAllocCallback   = NULL;
	pparam.vertexUsseFreeCallback    = NULL;
	pparam.vertexUsseMem             = patcherCombinedUsse;
	pparam.vertexUsseMemSize         = PATCHER_COMBINED_USSE_SIZE;
	pparam.vertexUsseOffset          = patcherVertexUsseOffset;
	pparam.fragmentUsseAllocCallback = NULL;
	pparam.fragmentUsseFreeCallback  = NULL;
	pparam.fragmentUsseMem           = patcherCombinedUsse;
	pparam.fragmentUsseMemSize       = PATCHER_COMBINED_USSE_SIZE;
	pparam.fragmentUsseOffset        = patcherFragmentUsseOffset;

	s_shaderPatcher = NULL;
	sceGxmShaderPatcherCreate(&pparam, &s_shaderPatcher);


	sceGxmShaderPatcherRegisterProgram(s_shaderPatcher, &_binary_clear_v_gxp_start, &s_vertexPidCls);
	sceGxmShaderPatcherRegisterProgram(s_shaderPatcher, &_binary_clear_f_gxp_start, &s_fragmentPidCls);
	sceGxmShaderPatcherRegisterProgram(s_shaderPatcher, &_binary_fixedlight_v_gxp_start, &s_vertexPid);
	sceGxmShaderPatcherRegisterProgram(s_shaderPatcher, &_binary_fixedlight_f_gxp_start, &s_fragmentPid);

	// create clear vertex format
	attr[0].streamIndex    = 0;
	attr[0].offset         = 0;
	attr[0].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attr[0].componentCount = 2;
	attr[0].regIndex       = get_attribute(s_vertexPidCls, SHADER_CLEAR_V_INPUT_PARAM_POSITOIN);

	stream[0].stride      = sizeof(VertexV32XY);
	stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	s_vertexPgmCls = NULL;
	sceGxmShaderPatcherCreateVertexProgram(s_shaderPatcher,
		s_vertexPidCls,
		attr,		1,
		stream,		1,
		&s_vertexPgmCls);

	s_fragmentPgmCls = NULL;
	sceGxmShaderPatcherCreateFragmentProgram(s_shaderPatcher,
		s_fragmentPidCls,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		NULL,
		sceGxmShaderPatcherGetProgramFromId(s_vertexPidCls),
		&s_fragmentPgmCls);

	s_vertexBufCls = (VertexV32XY *)g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		3 * sizeof(VertexV32XY), 4,
		SCE_GXM_MEMORY_ATTRIB_READ, &s_vertexUidCls);

	s_indexBufCls = (SceUInt16 *)g_alloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		3 * sizeof(SceUInt16), 2,
		SCE_GXM_MEMORY_ATTRIB_READ, &s_indexUidCls);

	s_vertexBufCls[0] = (VertexV32XY){-1.0f, -1.0f};
	s_vertexBufCls[1] = (VertexV32XY){ 3.0f, -1.0f};
	s_vertexBufCls[2] = (VertexV32XY){-1.0f,  3.0f};

	s_indexBufCls[0] = 0;
	s_indexBufCls[1] = 1;
	s_indexBufCls[2] = 2;

	gxm_initialized = 1;

	return 0;
}

int gxm_fini(void){

	int i;

	if(!gxm_initialized){
		return 0;
	}

	sceGxmFinish(s_context);

	g_free(s_indexUidCls);
	g_free(s_vertexUidCls);

	sceGxmShaderPatcherReleaseFragmentProgram(s_shaderPatcher, s_fragmentPgm);
	sceGxmShaderPatcherReleaseVertexProgram(s_shaderPatcher,   s_vertexPgm);
	sceGxmShaderPatcherReleaseFragmentProgram(s_shaderPatcher, s_fragmentPgmCls);
	sceGxmShaderPatcherReleaseVertexProgram(s_shaderPatcher,   s_vertexPgmCls);

	g_free(s_depthUid);

	sceGxmDisplayQueueFinish();
	for (i=0; i<DISPLAY_BUFFER_COUNT; i++) {
		sceGxmSyncObjectDestroy(s_dispSync[i]);
		g_free(s_dispUid[i]);
	}

	sceGxmShaderPatcherUnregisterProgram(s_shaderPatcher, s_fragmentPid);
	sceGxmShaderPatcherUnregisterProgram(s_shaderPatcher, s_vertexPid);
	sceGxmShaderPatcherUnregisterProgram(s_shaderPatcher, s_fragmentPidCls);
	sceGxmShaderPatcherUnregisterProgram(s_shaderPatcher, s_vertexPidCls);
	sceGxmShaderPatcherDestroy(s_shaderPatcher);
	combined_usse_free(s_patcherCombinedUsseUid);
	g_free(s_patcherBufUid);

	sceGxmDestroyRenderTarget(s_renderTarget);

	sceGxmDestroyContext(s_context);

	fragment_usse_free(s_fragmentUsseRingBufUid);
	g_free(s_fragmentRingBufUid);
	g_free(s_vertexRingBufUid);
	g_free(s_vdmRingBufUid);
	free(s_contextHost);

	sceGxmTerminate();

	gxm_initialized = 0;

	return 0;
}


void render_message_dialog(void *color, void *depth, void *sync){

	SceCommonDialogUpdateParam	updateParam;

	memset(&updateParam, 0, sizeof(updateParam));
	updateParam.renderTarget.colorFormat    = SCE_GXM_COLOR_FORMAT_A8B8G8R8;
	updateParam.renderTarget.surfaceType    = SCE_GXM_COLOR_SURFACE_LINEAR;
	updateParam.renderTarget.width          = DISPLAY_WIDTH;
	updateParam.renderTarget.height         = DISPLAY_HEIGHT;
	updateParam.renderTarget.strideInPixels = DISPLAY_STRIDE;

	updateParam.renderTarget.colorSurfaceData = color;
	updateParam.renderTarget.depthSurfaceData = depth;
	updateParam.displaySyncObject = (SceGxmSyncObject *)sync;

	sceCommonDialogUpdate(&updateParam);
}

void gxm_draw_start(void){
	sceGxmBeginScene(s_context, 0, s_renderTarget, NULL, NULL, s_dispSync[s_dispBack], &s_dispSurface[s_dispBack], &s_depthSurface);
}

void gxm_draw_end(void){
	sceGxmEndScene(s_context, NULL, NULL);
}

void gxm_clear_screen(void){

	sceGxmSetVertexProgram(s_context, s_vertexPgmCls);
	sceGxmSetFragmentProgram(s_context, s_fragmentPgmCls);
	sceGxmSetVertexStream(s_context, 0, s_vertexBufCls);
	sceGxmDraw(s_context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, s_indexBufCls, 3);

	sceGxmSetVertexProgram(s_context, s_vertexPgm);
	sceGxmSetFragmentProgram(s_context, s_fragmentPgm);

}

void gxm_swap_screen(void){
	Display display;

	// heartbeat function for Performance Analyzer
	sceGxmPadHeartbeat(&s_dispSurface[s_dispBack], s_dispSync[s_dispBack]);

	// add entry to display queue
	display.address = s_dispBuf[s_dispBack];
	sceGxmDisplayQueueAddEntry(s_dispSync[s_dispFront], s_dispSync[s_dispBack], &display);

	s_dispFront = s_dispBack;
	s_dispBack = (s_dispBack + 1) % DISPLAY_BUFFER_COUNT;
}

void gxm_update(void){
	gxm_draw_start();
	gxm_clear_screen();
	gxm_draw_end();
	render_message_dialog(s_dispBuf[s_dispBack], s_depthBuf, s_dispSync[s_dispBack]);
	gxm_swap_screen();
}