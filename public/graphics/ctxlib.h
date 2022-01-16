/*
 *
 *	ctxlib.h ( Authored by p0lyh3dron )
 *
 *
 *
 *
 */
#pragma once

#include <stdio.h>
#include <malloc.h>

#include <errno.h>
#include <string.h>

#define PARSE( var, type, buf, p ) var = *( type* )( buf + p ); p += sizeof( type )

#define SIG ( 'X' << 24 | 'E' << 16 | 'T' << 8 | 'C' )

#define VK_FORMAT_R32G32B32_SFLOAT 106

typedef unsigned int  u32;
typedef unsigned char byte;

const static u32 gRevision = 0;

typedef struct {
	u32        aRevision;
        char       aSig[ 4 ];
	u32        aFlags;
	u32        aWidth;
	u32        aHeight;
	u32        aLevels;
	u32        aFormat;
	u32        aFrames;
}ctx_header_t;

typedef struct {
	u32   aSize;
	char *apData;
}ctx_image_t;

typedef struct {
	ctx_header_t   aHead;
	ctx_image_t  **appImages;
	char          *apBuf;
}ctx_t;
/* Parses a .ctx file.  */
ctx_t *ctx_open( const char *spPath ) {
	FILE *pF = fopen( spPath, "rb" );
	if ( !pF ) {
		fprintf( stderr, "ctxlib.h: Failed to read %s\n", spPath );
		
		return 0;
	}
	fseek( pF, 0, SEEK_END );

	ctx_t *pTex    = ( ctx_t* )malloc( 1 * sizeof( ctx_t ) );
	u32   fileSize = ftell( pF );
	u32   p        = 0;
	
	pTex->apBuf    = ( char* )malloc( fileSize );
	char *pBuf     = pTex->apBuf;
	
	fseek( pF, 0, SEEK_SET );
	if ( fread( pTex->apBuf, 1, fileSize, pF ) != fileSize * 1 ) {
		fprintf( stderr, "ctxlib.h: Failed to read file contents of %s: %s\n", spPath, strerror( errno ) );

		return 0;
	}
	fclose( pF );
	/* Jeremy.  */
	pTex->apBuf = pBuf;

	PARSE( pTex->aHead, ctx_header_t, pBuf, p );

	if ( *( u32* )pTex->aHead.aSig != SIG ) {
		fprintf( stderr, "ctxlib.h: %s is not a valid .ctx file!\n", spPath );

		return 0;
	}

	pTex->appImages = ( ctx_image_t** )malloc( pTex->aHead.aFrames * sizeof( ctx_image_t* ) );
	for ( int i = 0; i < pTex->aHead.aFrames; ++i ) {
		pTex->appImages[ i ] = ( ctx_image_t* )malloc( pTex->aHead.aLevels * sizeof( ctx_image_t ) );
	}

	for ( int i = 0; i < pTex->aHead.aFrames; ++i ) {
		for ( int j = 0; j < pTex->aHead.aLevels; ++j ) {
			PARSE( pTex->appImages[ i ][ j ].aSize, u32, pBuf, p );

			pTex->appImages[ i ][ j ].apData = pBuf + p;
		}
	}

	return pTex;
}
/* Writes a .ctx file to the disk.  */
void ctx_write( const char *spPath, ctx_t *spTex ) {
	FILE *pF = fopen( spPath, "wb" );
	if ( !pF ) {
		fprintf( stderr, "Failed to write to file: %s\n", spPath );

		return;
	}
	fwrite( &spTex->aHead, 1, sizeof( spTex->aHead ), pF );
	for ( int i = 0; i < spTex->aHead.aFrames; ++i ) {
		for ( int j = 0; j < spTex->aHead.aLevels; ++j ) {
			fwrite( &spTex->appImages[ i ][ j ].aSize, 1, sizeof( spTex->appImages[ i ][ j ].aSize ), pF );
			fwrite( spTex->appImages[ i ][ j ].apData, 1, spTex->appImages[ i ][ j ].aSize, pF );
		}
	}
	fclose( pF );
}
/* Frees ctx_t allocated memory.  */
void ctx_free( ctx_t *spTex ) {
	free( spTex->apBuf );
	for ( int i = 0; i < spTex->aHead.aFrames; ++i ) {
	        free( spTex->appImages[ i ] );
	}
	free( spTex->appImages );
	free( spTex );
}
