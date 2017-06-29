/*
 * filesystem.h
 *
 *  Created on: Jun 29, 2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

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

#endif /* FILESYSTEM_H_ */
