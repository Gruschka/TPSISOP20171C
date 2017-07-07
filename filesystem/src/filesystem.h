/*
 * filesystem.h
 *
 *  Created on: Jun 29, 2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <commons/bitarray.h>


#define METADATA_DIR "/Metadata/";

typedef struct metadata{
	int blockAmount;
	int blockSize;
	char *magicNumber;
}t_FSMetadata;

typedef struct FS{
	DIR *mountDirectory;
	DIR *metadataDirectory;
	DIR *filesDirectory;
	DIR *dataDirectory;
	char *mountDirectoryPath;
	char *MetadataDirectoryPath;
	char *FSMetadataFileName;
	char *bitmapFileName;
	char *filesDirectoryPath;
	char *dataDirectoryPath;
	t_bitarray *bitmap;
	t_FSMetadata metadata;
}t_FS;
/*
Validar Archivo
Parámetros: [Path]
Cuando el Proceso Kernel reciba la operación de abrir un archivo deberá validar que el archivo exista.
Crear Archivo
Parámetros: [Path]
Cuando el Proceso Kernel reciba la operación de abrir un archivo deberá, en caso que el archivo no exista y este sea abierto en modo de creación (“c”), llamar a esta operación que creará el archivo dentro del path solicitado. Por default todo archivo creado se le debe asignar al menos 1 bloque de datos.
Borrar
Parámetros: [Path]
Borrará el archivo en el path indicado, eliminando su archivo de metadata y marcando los bloques como libres dentro del bitmap.
Obtener Datos
Parámetros: [Path, Offset, Size]
Ante un pedido de datos el File System devolverá del path enviado por parámetro, la cantidad de bytes definidos por el Size a partir del offset solicitado. Requiere que el archivo se encuentre abierto en modo lectura (“r”).
Guardar Datos
Parámetros: [Path, Offset, Size, Buffer]
Ante un pedido de escritura el File System almacenará en el path enviado por parámetro, los bytes del buffer, definidos por el valor del Size y a partir del offset solicitado. Requiere que el archivo se encuentre abierto en modo escritura (“w”).
*/

int fs_loadConfig(t_FS *FS);
int fs_mount(t_FS *FS);
int fs_openOrCreateMetadata(t_FS *FS);
int fs_openOrCreateMetadataFiles(t_FS *FS, int blockSize, int blockAmount, char *magicNumber);
t_FSMetadata fs_getMetadataFromFile(FILE *fileDescriptor);
int fs_validateFile(char *path);
int fs_createFile(char *path);
int fs_createBlockFile(int blockNumber);
int fs_getFirstFreeBlock(t_FS *FS);
int fs_getNumberOfDigits(int number);


extern t_FS myFS;

#endif /* FILESYSTEM_H_ */
