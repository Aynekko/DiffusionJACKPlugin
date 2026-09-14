// Diffusion cable rendering for JACK

#include <stdio.h>
#include <locale.h>

// Plugin API
#include "PluginMeta.h"
#include "PluginActions.h"
#include "PluginEntity.h"
#include "PluginWorld.h"

plugin_funcs_t gEditorfuncs;

#define VectorMA( a, scale, b, c ) ( ( c )[0] = ( a )[0] + ( scale ) * ( b )[0], ( c )[1] = ( a )[1] + ( scale ) * ( b )[1], ( c )[2] = ( a )[2] + ( scale ) * ( b )[2] )

/*
===============
vpMain
Application entry point
===============
*/
DLL_EXPORT int vpMain( plugin_funcs_t *editorFuncs, int editorPluginVersion )
{
	if ( editorFuncs->m_intefaceVersion < sizeof( plugin_funcs_t ) )
		return -1;

	if ( editorPluginVersion != PLUGIN_VERSION )
		return PLUGIN_VERSION;

	memcpy( &gEditorfuncs, editorFuncs, editorFuncs->m_intefaceVersion );
	setlocale( LC_ALL, "C" );
	return 0;
}

DLL_EXPORT int vpEnumModelFormats( pfnRegisterIOFormat reg, void *lib )
{
	// Register a fake extension so the editor will call us
	reg( 0, "Cable Dummy", ".cable", lib );
	
	return 1;
}

DLL_EXPORT bool vpLoadModel( int formatIndex, const char *filePath, byte *buf, int bufSize, qStudioData_s *studioData )
{
	// Just mark it as loaded – no real geometry needed
	studioData->m_dataPtr = (void *)1; // any non-null value
	studioData->m_bboxMin = vec3_t( -16, -16, -16 );
	studioData->m_bboxMax = vec3_t( 16, 16, 16 );
	return true;
}

DLL_EXPORT void vpUnloadModel( int formatIndex, qStudioData_s *studioData )
{
	studioData->m_dataPtr = nullptr;
}

DLL_EXPORT bool vpGetModelBounds( int formatIndex, vec3_t *mins, vec3_t *maxs, unsigned int flags, qStudioData_s *studioData, qEntity_s *ent )
{
	const vec3_t v_mins( -16, -16, -16 );
	const vec3_t v_maxs( 16, 16, 16 );

	if ( mins )
		*mins = v_mins;
	if ( maxs )
		*maxs = v_maxs;

	if ( ent )
	{
		ent->m_bboxMin = v_mins;
		ent->m_bboxMax = v_maxs;
	}
	return true;
}

DLL_EXPORT bool vpSetPalette( int formatIndex, byte *palette /* 768 bytes RGB */ )
{
	// ignore the palette for untextured / solid-colour cables
	return true;
}

// Return true so JACK treats the format as a real studiomodel
// (adds the internal 0x8000 flag, enables model rendering path)
DLL_EXPORT bool vpGetModelFormatFlags( int formatIndex )
{
	return true;
}

const char *Entity_GetKeyValue( qEntity_s *ent, const char *key )
{
	if ( !ent || !key )
		return nullptr;

	for ( epair_t *ep = ent->epairs; ep; ep = ep->next )
	{
		if ( ep->key && !stricmp( ep->key, key ) )
			return ep->value; // may be nullptr
	}
	return nullptr;
}

float Entity_GetKeyValueFloat( qEntity_s *ent, const char *key, float defaultValue = 0.0f )
{
	const char *val = Entity_GetKeyValue( ent, key );
	if ( !val || !val[0] )
		return defaultValue;

	return (float)atof( val );
}

// Parse a keyvalue string "x y z" into a vec3_t.
bool Entity_GetKeyValueVector( qEntity_s *ent, const char *key, vec3_t &out )
{
	const char *val = Entity_GetKeyValue( ent, key );
	if ( !val || !val[0] )
		return false;

	float x = 0.f, y = 0.f, z = 0.f;
	if ( sscanf( val, "%f %f %f", &x, &y, &z ) < 3 )
		return false;

	out.x = x;
	out.y = y;
	out.z = z;
	return true;
}

// Resolve the entity named in the "target" key and return its origin.
// Returns true if a target entity was found.
bool Entity_GetTargetOrigin( qEntity_s *ent, vec3_t &outOrigin )
{
	if ( !ent )
		return false;

	// 1. Read the target name from the keyvalue
	const char *targetName = Entity_GetKeyValue( ent, "target" );
	if ( !targetName || !targetName[0] )
		return false;

	// 2. Get the current world
	qWorld_s *world = Global_GetCurrentWorld();
	if ( !world || !world->m_entityList )
		return false;

	// 3. Find the entity that has that targetname
	qEntity_s *targetEnt = Entity_FindByTargetname( world->m_entityList, targetName );
	if ( !targetEnt )
		return false;

	// 4. Copy its origin
	outOrigin = targetEnt->m_vecOrigin;
	return true;
}

DLL_EXPORT void vpRenderModel( int formatIndex, int renderFlags, qStudioData_s *studioData, qEntity_s *entityInfo )
{
	if ( !entityInfo )
		return;

	// Only care about the cables
	if ( stricmp( entityInfo->m_className, "env_cable" ) != 0 )
		return;

	// read parameters from the entity
	vec3_t start = entityInfo->m_vecOrigin;

//	float TargetSway = Entity_GetKeyValueFloat( entityInfo, "fuser3" ); // TODO add sway
	int NumberOfCables = (int)Entity_GetKeyValueFloat( entityInfo, "cables" );
	if ( NumberOfCables < 1 )
		NumberOfCables = 1;
	float falldepth = Entity_GetKeyValueFloat( entityInfo, "fuser1" );
	if ( falldepth <= 0.0f )
		falldepth = 0.001f;
	float AddSlack = Entity_GetKeyValueFloat( entityInfo, "addslack" );

	// cable end point
	// first, we look at the specified coordinate point
	// if it's not set, look for a specified target entity
	// if both are missing, don't draw anything (DRAW BOX????)
	vec3_t end = { 0, 0, 0 };
	if ( !Entity_GetKeyValueVector( entityInfo, "vuser1", end ) )
	{
		if ( !Entity_GetTargetOrigin( entityInfo, end ) )
			return;
	}
	
	const float fwidth = Entity_GetKeyValueFloat( entityInfo, "fuser2" );
	int segments = (int)Entity_GetKeyValueFloat( entityInfo, "iuser2" );

	// hardcoded limits
	if ( segments < 3 )
		segments = 3;
	else if ( segments > 49 )
		segments = 49;

	// Colour / alpha from the entity’s render properties
	byte r = entityInfo->m_renderMode.m_renderColor.r;
	byte g = entityInfo->m_renderMode.m_renderColor.g;
	byte b = entityInfo->m_renderMode.m_renderColor.b;
	byte a = entityInfo->m_renderMode.m_renderColor.a;
	if( a == 0 )
		a = 255;

	if( (r == 255 && g == 255 && b == 255) || (r == 0 && g == 0 && b == 0) )
	{
		// 1 1 1 is still rendered as white for whatever reason
		// and 0 0 0 or empty field is replaced as 255 255 255, so treat it as a black cable...
		r = 3;
		g = 3;
		b = 3;
	}

	// get the view info for billboarding
	viewInfo_t view;
	PR_GetViewInfo( &view );

	for ( int cablenum = 0; cablenum < NumberOfCables; cablenum++ )
	{
		if ( cablenum > 0 )
			falldepth += AddSlack * cablenum;

		vec3_t points[50]; // hardcoded
		vec3_t mid, drop;
		VectorMA( start, 0.5f, end - start, mid );
		drop = mid;
		drop.z -= falldepth;

		const int numPoints = segments + 1;

		for ( int i = 0; i < numPoints; i++ )
		{
			float f = (float)i / (float)segments;
			points[i] = start * ( ( 1.0f - f ) * ( 1.0f - f ) )
						+ drop * ( ( 1.0f - f ) * f * 2.0f )
						+ end * ( f * f );
		}

		// GL state
		unsigned int state = GLS_CULL_NONE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;

		if ( !( renderFlags & RFL_WIREFRAME ) )
			state |= GLS_DEPTHWRITE;

		PR_SetState( state );

		// optional shader?
		// PR_BindShader(yourWhiteShader);

		// rendering
		PR_Begin( PRIMTYPE_TRIANGLE_STRIP );

		for ( int i = 0; i < numPoints; i++ )
		{
			// simple view-facing right vector
			vec3_t tangent;
			if ( i == 0 )
				tangent = points[0] - points[1];
			else
				tangent = points[i - 1] - points[i];

			vec3_t toView = points[i] - view.r_origin;
			vec3_t right;
			CrossProduct( tangent, -toView, right );
			VectorNormalize( right );

			vec3_t leftVert, rightVert;
			VectorMA( points[i], fwidth, right, leftVert );
			VectorMA( points[i], -fwidth, right, rightVert );

			PR_Color4ub( r, g, b, a );
			PR_Vertex3fv( leftVert.Base() );

			PR_Color4ub( r, g, b, a );
			PR_Vertex3fv( rightVert.Base() );
		}

		PR_End();
	}
}

//===========================================================================================
// Hiding cubemap_box entities
//===========================================================================================
static bool g_hideCubemapBoxes = false;
static void ToggleHideCubemapBoxes( int actionUserData )
{
	qWorld_s *world = Global_GetCurrentWorld();
	if ( !world || !world->m_entityList )
		return;

	g_hideCubemapBoxes = !g_hideCubemapBoxes;

	// Walk every entity
	for ( qEntity_s *e = world->m_entityList; e; e = e->next )
	{
		if ( !e->m_className || stricmp( e->m_className, "cubemap_box" ) != 0 )
			continue;

		if ( g_hideCubemapBoxes )
			e->m_editorFlags |= EFL_HIDDEN;
		else
			e->m_editorFlags &= ~EFL_HIDDEN;
	}

	Sys_Printf( g_hideCubemapBoxes
					? "cubemap_box entities hidden"
					: "cubemap_box entities shown" );
}

static pluginActionDesc_t hideCubemapBoxesAction = {
#if JACK_API_VERSION >= API_VERSION_STEAM_BETA
	"HideCubemapBoxes", // internal name
#endif
	"&Hide cubemap_box", // menu / toolbar title (& = accelerator)
	"Toggle visibility of cubemap_box entities",
	"Diffusion", // category (submenu name)
#if JACK_API_VERSION <= API_VERSION_STEAM_PUBLIC
	0,
#endif
	ACTION_FLAG_INLEVEL, // enabled only when a map is open
	0,					 // userData passed to the callback
	ToggleHideCubemapBoxes };

DLL_EXPORT int vpEnumActions( pfnRegisterAction registerAction, void *pluginManager )
{
	registerAction( &hideCubemapBoxesAction, pluginManager );
	return 1;
}