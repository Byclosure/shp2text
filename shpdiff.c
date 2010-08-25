/******************************************************************************
 * $Id: shpdiff.c,v 1.2 2005/11/07 05:07:37 bryce Exp $
 *
 * Project:  Shapelib shpdiff
 * Purpose:  Application show differences between two shapefiles 
 *           in human readable form.
 * Author:   Bryce Nesbitt, http://www.obviously.com, bryce2 'ahht' obviously.com
 *
 * Limitations:  Can't diff a joined table.
 *
 * TODO:
 *       !! Figure out if shape compare really works, and
 *       match identical shapes even if primary key has changed.
 *
 *       Accept "columns to ignore" from the command line
 *       Accept "identification key" from the command line
 *       Accept "scanAhead" setting from command line
 *
 *       Diff shapefiles stored inside a zip/tar archive.
 *       Diff just a table, without associated shapes.
 *
 *       Summarize long lists of shape changes.
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 * Copyright (c) 2003, Bryce Nesbitt
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see LICENSE.LGPL).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: shpdiff.c,v $
 * Revision 1.2  2005/11/07 05:07:37  bryce
 * Minor tweaks for new GCC
 *
 * Revision 1.1  2004/04/23 15:10:34  bryce
 * shapelib library (basis of my shpdiff utility)
 *
 *
 */

static char rcsid[] = "$Id: shpdiff.c,v 1.2 2005/11/07 05:07:37 bryce Exp $";

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"
#include "assert.h"

#define unless(x)       if(!(x))
typedef  int bool;
#define true    1
#define false   0

void printSHP(SHPHandle iSHP, int iRecord);
void printDBF(DBFHandle iDBF, int iRecord);
bool compareDBF(DBFHandle iDBF, int iRecord, DBFHandle jDBF, int jRecord, bool print);
bool compareDBFkey(DBFHandle iDBF, int iRecord, DBFHandle jDBF, int jRecord, bool print);
bool compareSHP(SHPHandle iSHP, int iRecord, SHPHandle jSHP, int jRecord, bool print);
void printHeader(DBFHandle xDBF, SHPHandle xSHP);

int	bVerbose=0;
int	bMultiLine=1;

#define BUFFER_MAX      512
char    iBuffer[BUFFER_MAX];
char    jBuffer[BUFFER_MAX];
#define COLUMNS_MAX     100
int     columnMatchArray[COLUMNS_MAX];
#define SCAN_AHEAD      40  // How far to search when resyncronizing 

int         identifyKey;    // Key to display, to help human identify record :TODO:
int         primaryKey;     // Primary and unique key, to help computer identify record :TODO:


int main( int argc, char ** argv )
{
    SHPHandle	iSHP;
    DBFHandle   iDBF;

    SHPHandle	jSHP;
    DBFHandle   jDBF;

    int         i, j, k;
    int         iEntities, jEntities;
    int         iRecord,   jRecord;

    int         scanAhead;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 3 )
    {
	printf( "shpdiff original_shapefile comparison_shapefile\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile parts                                 */
/* -------------------------------------------------------------------- */
    iSHP = SHPOpen( argv[1], "rb" );
    if( iSHP == NULL )
    {
	printf( "Unable to find shapefile \"%s\"\n", argv[1] );
	exit( 1 );
    }
    iDBF = DBFOpen( argv[1], "rb" );
    if( iDBF == NULL )
    {
	printf( "Unable to find shapefile \"%s\"\n", argv[1] );
	exit( 1 );
    }
    if( DBFGetFieldCount(iDBF) == 0 )
    {
        printf( "There are no fields in table %s!\n", argv[1] );
        exit( 3 );
    }


    jSHP = SHPOpen( argv[2], "rb" );
    if( jSHP == NULL )
    {
	printf( "Unable to find shapefile \"%s\"\n", argv[2] );
        exit( 1 );
    }
    jDBF = DBFOpen( argv[2], "rb" );
    if( jDBF == NULL )
    {
	printf( "Unable to find shapefile \"%s\"\n", argv[2] );
        exit( 1 );
    }
    if( DBFGetFieldCount(jDBF) == 0 )
    {
        printf( "There are no fields in table %s!\n", argv[2] );
        exit( 3 );
    }


/* -------------------------------------------------------------------- */
/*      Print out some useful data about each shapefile                 */
/* -------------------------------------------------------------------- */
    printf("Original ");
    printHeader( iDBF, iSHP );
    printf("Comparison ");
    printHeader( jDBF, jSHP );
    printf("\n");


/* -------------------------------------------------------------------- */
/*      Guess which column headering might best identify each record    */
/* -------------------------------------------------------------------- */
    identifyKey = DBFGetFieldIndex( iDBF, "NAME" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = DBFGetFieldIndex( iDBF, "STREET" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = DBFGetFieldIndex( iDBF, "TOWN" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = DBFGetFieldIndex( iDBF, "ID" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = DBFGetFieldIndex( iDBF, "STATION" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = DBFGetFieldIndex( iDBF, "LINE" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = DBFGetFieldIndex( iDBF, "TRAILNAME" );
    if( identifyKey >= 0 )
        goto gotkey;
    identifyKey = -1;
gotkey:

/* -------------------------------------------------------------------- */
/*      Match up column indexes between the shapefiles                  */     
/* -------------------------------------------------------------------- */
{
int     i, temp;
int     nWidth, nDecimals;
char    szTitle[12];

    if(identifyKey)
    {
        DBFGetFieldInfo( iDBF, identifyKey, szTitle, &nWidth, &nDecimals );
        printf("NOTE: Using column %s to identify records\n", szTitle);
    }

    for( i = 0; i < DBFGetFieldCount(iDBF); i++ )
    {
        DBFGetFieldInfo( iDBF, i, szTitle, &nWidth, &nDecimals );
        columnMatchArray[i] = DBFGetFieldIndex( jDBF, szTitle );
        if( columnMatchArray[i] < 0 )
        {
                printf("WARNING: Column %s deleted from original\n", szTitle);
        }
    }
    for( i = 0; i < DBFGetFieldCount(jDBF); i++ )
    {
        DBFGetFieldInfo( jDBF, i, szTitle, &nWidth, &nDecimals );
        if( DBFGetFieldIndex( iDBF, szTitle ) < 0 )
        {
                printf("WARNING: Column \"%s\" was added\n", szTitle);
        }
    }

    // Eliminate certain columns from the comparison
    temp = DBFGetFieldIndex( jDBF, "LENGTH" );
    if( temp >= 0 )
    {
        columnMatchArray[temp] = -1;
        printf("WARNING: Ingoring column \"%s\"\n", "LENGTH");
    }
    temp = DBFGetFieldIndex( jDBF, "FNODE_" );
    if( temp >= 0 )
    {
        columnMatchArray[temp] = -1;
        printf("WARNING: Ingoring column \"%s\"\n", "FNODE");
    }
    temp = DBFGetFieldIndex( jDBF, "TNODE_" );
    if( temp >= 0 )
    {
        columnMatchArray[temp] = -1;
        printf("WARNING: Ingoring column \"%s\"\n", "TNODE_");
    }
    temp = DBFGetFieldIndex( jDBF, "AREA" );
    if( temp >= 0 )
    {
        columnMatchArray[temp] = -1;
        printf("WARNING: Ingoring column \"%s\"\n", "AREA");
    }
}


/* -------------------------------------------------------------------- */
/*      Main loop                                                       */
/* -------------------------------------------------------------------- */
    scanAhead= SCAN_AHEAD;
    iRecord = 0;
    jRecord = 0;
    iEntities = DBFGetRecordCount(iDBF);
    jEntities = DBFGetRecordCount(jDBF);

    while( (iRecord < iEntities) || (jRecord < jEntities) )
    {
        int shpDiff;
        int dbfDiff;
        int keyDiff;

        //printf("iRecord %d jRecord %d\n", iRecord, jRecord);


        /* Print out any extra records at the end of either file */
        if( iRecord == iEntities )
        {
            while( jRecord < jEntities )
            {
                printf("\nNew record %d found\n", jRecord);
                printf("-------------------------------");
                printDBF(jDBF, jRecord);
                jRecord++;
            }
            goto end;
        }

        if( jRecord == jEntities )
        {
            while( iRecord < iEntities )
            {
                printf("\nRecord %d: deleted from original\n", iRecord);
                printf("--------------------------------");
                printDBF(iDBF, iRecord);
                iRecord++;
            }
            goto end;
        }

        shpDiff = compareSHP   (iSHP, iRecord, jSHP, jRecord, false);
        dbfDiff = compareDBF   (iDBF, iRecord, jDBF, jRecord, false);
        keyDiff = compareDBFkey(iDBF, iRecord, jDBF, jRecord, false);
        //printf("Compare iRecord=%d jRecord=%d: shpDiff=%d dbfDiff=%d keyDiff=%d\n",
        //        iRecord, jRecord, shpDiff, dbfDiff, keyDiff);

        /* If everything is the same, skip ahead */
        if( !shpDiff && !dbfDiff && !keyDiff )
        {
            iRecord++;
            jRecord++;
            goto end;
        }

        /* If the primary key record is different, 
         * and the shape has also changed, scan ahead for a better match */
        if ( keyDiff )
        {
            for( i=1; (i<scanAhead) && (iRecord+i < iEntities); i++ )
            {
                shpDiff = compareSHP   (iSHP, iRecord+i, jSHP, jRecord, false);
                dbfDiff = compareDBF   (iDBF, iRecord+i, jDBF, jRecord, false);
                keyDiff = compareDBFkey(iDBF, iRecord+i, jDBF, jRecord, false);
                //printf("Scan iRecord=%d jRecord=%d: shpDiff=%d dbfDiff=%d keyDiff=%d\n",
                //    iRecord+i, jRecord, shpDiff, dbfDiff, keyDiff);

                if ( !shpDiff || !dbfDiff )
                {
                    for( j=0; j<i; j++ )
                    {
                        printf("\nRecord %d: deleted from original\n", iRecord);
                        printf("--------------------------------");
                        printDBF(iDBF, iRecord);
                        iRecord++;
                    }
                    goto end;
                }
            }

            /* We can't find any match, call it a "new" record */
            printf("\nNew record %d found\n", jRecord);
            printf("-------------------------------");
            printDBF(jDBF, jRecord);
            jRecord++;
            goto end;
        }

        /* Either the shape or the primary key match exactly, show a change record */
        printf("\nRecord %d:", iRecord);
        if( identifyKey )
            printf(" (%s) ", DBFReadStringAttribute( iDBF, iRecord, identifyKey ));
        if( shpDiff )
            printf("shape change\n");
        else
            printf("\n");
        if( dbfDiff )
        {
            compareDBF(iDBF, iRecord, jDBF, jRecord, true);
        }
        iRecord++;
        jRecord++;

        end:;
    }

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    SHPClose( iSHP );
    DBFClose( iDBF );
    SHPClose( jSHP );
    DBFClose( jDBF );

#ifdef USE_DBMALLOC
    malloc_dump(2);
#endif

    exit( 0 );
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*	Print a human readable list of a shape record                   */
/* -------------------------------------------------------------------- */
void printSHP(SHPHandle iSHP, int iRecord)
{
const char 	*pszPlus;
SHPObject	*psShape;
int             j, iPart;

	psShape = SHPReadObject( iSHP, iRecord );

	printf( "%s", SHPTypeName(psShape->nSHPType));

	for( j = 0, iPart = 1; j < psShape->nVertices; j++ )
	{
            const char	*pszPartType = "";

            if( j == 0 && psShape->nParts > 0 )
                pszPartType = SHPPartTypeName( psShape->panPartType[0] );
            
	    if( iPart < psShape->nParts
                && psShape->panPartStart[iPart] == j )
	    {
                pszPartType = SHPPartTypeName( psShape->panPartType[iPart] );
		iPart++;
		pszPlus = "+";
	    }
	    else
	        pszPlus = " ";

	    printf("%s%.3f-%.3f",
                   pszPlus,
                   psShape->padfX[j],
                   psShape->padfY[j] );

            if( psShape->padfZ[j] || psShape->padfM[j] )
            {
                   printf("(%g, %g) %s \n",
                   psShape->padfZ[j],
                   psShape->padfM[j],
                   pszPartType );
            }
	}

        SHPDestroyObject( psShape );
}

bool compareSHP(SHPHandle iSHP, int iRecord, SHPHandle jSHP, int jRecord, bool print)
{
SHPObject       *piShape;
SHPObject       *pjShape;
int             j;
int             mismatch;

        mismatch = 0;

        piShape = SHPReadObject( iSHP, iRecord );
        pjShape = SHPReadObject( jSHP, jRecord );

        if( print )
            printf("Vertices %d / %d\n", piShape->nVertices, pjShape->nVertices);

        if( piShape->nVertices != pjShape->nVertices )
            return( true );

        for( j = 0; j < piShape->nVertices; j++ )
        {
            const char  *pszPartType = "";

            if( piShape->padfX[j] != pjShape->padfX[j] )
                mismatch++;
            if( piShape->padfY[j] != pjShape->padfY[j] )
                mismatch++;
            if( piShape->padfZ[j] != pjShape->padfZ[j] )
                mismatch++;
            if( piShape->padfM[j] != pjShape->padfM[j] )
                mismatch++;
        }
        SHPDestroyObject( piShape );
        SHPDestroyObject( pjShape );

        if( print )
            printf("Mismatches %d\n", mismatch);

        return( mismatch? true:false );
}


/* -------------------------------------------------------------------- */
/*      Print a human readable list of a dbf database record            */
/* -------------------------------------------------------------------- */
void printDBF(DBFHandle iDBF, int iRecord)
{
    int         i;
    char        szTitle[12];            // Column heading
    int         nWidth, nDecimals;
    char        szFormat[32];

        for( i = 0; i < DBFGetFieldCount(iDBF); i++ )
        {
            DBFFieldType        eType;
            eType = DBFGetFieldInfo( iDBF, i, szTitle, &nWidth, &nDecimals );

            if( bMultiLine )
            {
                printf( "\n%s: ", szTitle );
            } else
            {
               printf("|");
            }
            sprintf( szFormat, "%%-%ds", nWidth );
            printf( szFormat, DBFReadStringAttribute( iDBF, iRecord, i ) );
        }
        printf("\n");
}


bool compareDBF(DBFHandle iDBF, int iRecord, DBFHandle jDBF, int jRecord, bool print)
{
int             i,j;
int             temp;
bool            result;
DBFFieldType    eType;
char            szTitle[12];            // Column heading
int             nWidth, nDecimals;

        result = false;
        for( i = 0; i < DBFGetFieldCount(iDBF); i++ )
        {
            j = columnMatchArray[i];
            if( j < 0 )
                continue;
            eType = DBFGetFieldInfo( iDBF, i, szTitle, &nWidth, &nDecimals );
            assert( nWidth < BUFFER_MAX );
            memcpy( &iBuffer, DBFReadStringAttribute( iDBF, iRecord, i ), nWidth );
            memcpy( &jBuffer, DBFReadStringAttribute( jDBF, jRecord, j ), nWidth );

            /* Convert numbers, compare everything else as a string */
            if( eType == FTDouble )
                temp = (atof( iBuffer ) != atof( jBuffer ) );
            else if( eType == FTInteger )
                temp = (atoi( iBuffer ) != atoi( jBuffer ) );
            else
                temp = memcmp ( &iBuffer, &jBuffer, nWidth );

            /* Unless they are both NULL, flag record as different */
            if( temp )
            {
                result = true;
                if( DBFIsAttributeNULL( iDBF, iRecord, i ) &&
                    DBFIsAttributeNULL( jDBF, jRecord, j ) )
                        result = false;
            }

            if( temp && print ) {
                //printf("%d wide record %d and %d differ (etype=%d)\n", nWidth, iRecord, jRecord, eType );
                iBuffer[ nWidth ] = 0;
                jBuffer[ nWidth ] = 0;
                printf("%s: %s >>> %s\n", szTitle,  &iBuffer,  &jBuffer);
                }
        }
        return( result );
}


/* Compare our assumed "primary key", or identifying record */
bool compareDBFkey(DBFHandle iDBF, int iRecord, DBFHandle jDBF, int jRecord, bool print)
{
int             i,j;
int             temp;
bool            result;
DBFFieldType    eType;
char            szTitle[12];            // Column heading
int             nWidth, nDecimals;

        result = false;

        if( identifyKey >= 0 )
        {
            i = identifyKey;
            j = columnMatchArray[i];

            eType = DBFGetFieldInfo( iDBF, i, szTitle, &nWidth, &nDecimals );
            assert( nWidth < BUFFER_MAX );
            memcpy( &iBuffer, DBFReadStringAttribute( iDBF, iRecord, i ), nWidth );
            memcpy( &jBuffer, DBFReadStringAttribute( jDBF, jRecord, j ), nWidth );
            temp = memcmp ( &iBuffer, &jBuffer, nWidth );
            if( temp )
                result = true;
        }
        return( result );
}

/* -------------------------------------------------------------------- */
/*      Print basic file data                                           */
/* -------------------------------------------------------------------- */
void printHeader(DBFHandle xDBF, SHPHandle xSHP)
{
double 	adfMinBound[4], adfMaxBound[4];
int     xEntities;
int     xShapeType;
int     nWidth, nDecimals;
int     i;
char    szTitle[12];

        SHPGetInfo( xSHP, &xEntities, &xShapeType, adfMinBound, adfMaxBound );
        printf( "Shapefile Type: %s, %d shapes ",
            SHPTypeName( xShapeType ), xEntities );

        printf( "%d database records\n", DBFGetRecordCount(xDBF) );
        if( bVerbose ) 
        {
            for( i = 0; i < DBFGetFieldCount(xDBF); i++ )
            {
                DBFFieldType        eType;
                const char          *pszTypeName;

                eType = DBFGetFieldInfo( xDBF, i, szTitle, &nWidth, &nDecimals );
                if( eType == FTString )
                    pszTypeName = "String";
                else if( eType == FTInteger )
                    pszTypeName = "Integer";
                else if( eType == FTDouble )
                    pszTypeName = "Double";
                else if( eType == FTInvalid )
                    pszTypeName = "Invalid";

                printf( "Field %d: Type=%s, Title=`%s', Width=%d, Decimals=%d\n",
                        i, pszTypeName, szTitle, nWidth, nDecimals );
            }
        }
}

