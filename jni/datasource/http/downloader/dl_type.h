/*************************************************************************
  > File Name: dl_type.h
  > Author: liyunlong
  > Mail: liyunlong_88@.com 
  > Created Time: 年月1日 星期四 16时40分10秒
 ************************************************************************/


#ifndef DL_Types_h
#define DL_Types_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /** The DL_API and DL_APIENTRY are platform specific definitions used
     *  to declare DL function prototypes.  They are modified to meet the
     *  requirements for a particular platform */
#      ifdef __DL_EXPORTS
#          define DL_API
#      else
#          define DL_API extern
#      endif

#ifndef DL_APIENTRY
#define DL_APIENTRY
#endif

    /** DL_IN is used to identify inputs to an DL function.  This designation
      will also be used in the case of a pointer that points to a parameter
      that is used as an output. */
#ifndef DL_IN
#define DL_IN
#endif

    /** DL_OUT is used to identify outputs from an DL function.  This
      designation will also be used in the case of a pointer that points
      to a parameter that is used as an input. */
#ifndef DL_OUT
#define DL_OUT
#endif


    /** DL_INOUT is used to identify parameters that may be either inputs or
      outputs from an DL function at the same time.  This designation will
      also be used in the case of a pointer that  points to a parameter that
      is used both as an input and an output. */
#ifndef DL_INOUT
#define DL_INOUT
#endif

    /** DL_ALL is used to as a wildcard to select all entities of the same type
     *  when specifying the index, or referring to a object by an index.  (i.e.
     *  use DL_ALL to indicate all N channels). When used as a port index
     *  for a config or parameter this DL_ALL denotes that the config or
     *  parameter applies to the entire component not just one port. */
#define DL_ALL xFFFFFFFF


    /** @defgroup metadata Metadata handling
     *
     */

    /** DL_U8 is an  bit unsigned quantity that is byte aligned */
    typedef unsigned char DL_U8;

    /** DL_S8 is an  bit signed quantity that is byte aligned */
    typedef signed char DL_S8;

    /** DL_U16 is a  bit unsigned quantity that is  bit word aligned */
    typedef unsigned short DL_U16;

    /** DL_S16 is a  bit signed quantity that is  bit word aligned */
    typedef signed short DL_S16;

    /** DL_U32 is a  bit unsigned quantity that is  bit word aligned */
    typedef unsigned long DL_U32;

    /** DL_S32 is a  bit signed quantity that is  bit word aligned */
    typedef signed long DL_S32;


    /** DL_U64 is a  bit unsigned quantity that is  bit word aligned */
    /** DL_U64 is a  bit unsigned quantity that is  bit word aligned */
    typedef unsigned long long DL_U64;

    /** DL_S64 is a  bit signed quantity that is  bit word aligned */
    typedef signed long long DL_S64;


    /** The DL_BOOL type is intended to be used to represent a true or a false
      value when passing parameters to and from the DL core and components.  The
      DL_BOOL is a  bit quantity and is aligned on a  bit word boundary.
      */
    typedef enum DL_BOOL {
        DL_FALSE = 0,
        DL_TRUE = !DL_FALSE,
        DL_BOOL_MAX = 0x7FFFFFFF
    } DL_BOOL;

    /** The DL_PTR type is intended to be used to pass pointers between the DL
      applications and the DL Core and components.  This is a  bit pointer and
      is aligned on a  bit boundary.
      */
    typedef void* DL_PTR;

    /** The DL_STRING type is intended to be used to pass "C" type strings between
      the application and the core and component.  The DL_STRING type is a 
      bit pointer to a zero terminated string.  The  pointer is word aligned and
      the string is byte aligned.
      */
    typedef char* DL_STRING;
    typedef const char* DL_CONST_STRING;

    /** The DL_BYTE type is intended to be used to pass arrays of bytes such as
      buffers between the application and the component and core.  The DL_BYTE
      type is a  bit pointer to a zero terminated string.  The  pointer is word
      aligned and the string is byte aligned.
      */
    typedef unsigned char* DL_BYTE;

    /** DL_UUIDTYPE is a very long unique identifier to uniquely identify
      at runtime.  This identifier should be generated by a component in a way
      that guarantees that every instance of the identifier running on the system
      is unique. */
    typedef unsigned char DL_UUIDTYPE[128];

    /** The DL_ENDIANTYPE enumeration is used to indicate the bit ordering
      for numerical data (i.e. big endian, or little endian).
      */
    typedef enum DL_ENDIANTYPE
    {
        DL_EndianBig, /**< big endian */
        DL_EndianLittle, /**< little endian */
        DL_EndianMax = 0x7FFFFFFF
    } DL_ENDIANTYPE;


    /** The DL_NUMERICALDATATYPE enumeration is used to indicate if data
      is signed or unsigned
      */
    typedef enum DL_NUMERICALDATATYPE
    {
        DL_NumericalDataSigned, /**< signed data */
        DL_NumericalDataUnsigned, /**< unsigned data */
        DL_NumercialDataMax = 0x7FFFFFFF
    } DL_NUMERICALDATATYPE;


    /** Unsigned bounded value type */
    typedef struct DL_BU32 {
        DL_U32 nValue; /**< actual value */
        DL_U32 nMin;   /**< minimum for value (i.e. nValue >= nMin) */
        DL_U32 nMax;   /**< maximum for value (i.e. nValue <= nMax) */
    } DL_BU32;


    /** Signed bounded value type */
    typedef struct DL_BS32 {
        DL_S32 nValue; /**< actual value */
        DL_S32 nMin;   /**< minimum for value (i.e. nValue >= nMin) */
        DL_S32 nMax;   /**< maximum for value (i.e. nValue <= nMax) */
    } DL_BS32;


    typedef struct DL_TICKS
    {
        DL_U32 nLowPart;    /** low bits of the signed  bit tick value */
        DL_U32 nHighPart;   /** high bits of the signed  bit tick value */
    } DL_TICKS;
#define DL_TICKS_PER_SECOND 1000000

    /** Define the public interface for the DL Handle.  The core will not use
      this value internally, but the application should only use this value.
      */
    typedef void* DL_HANDLETYPE;

        /** The DL_VERSIONTYPE union is used to specify the version for
      a structure or component.  For a component, the version is entirely
      specified by the component vendor.  Components doing the same function
      from different vendors may or may not have the same version.  For
      structures, the version shall be set by the entity that allocates the
      structure.  For structures specified in the DL . specification, the
      value of the version shall be set to ... in all cases.  Access to the
      DL_VERSIONTYPE can be by a single  bit access (e.g. by nVersion) or
      by accessing one of the structure elements to, for example, check only
      the Major revision.
      */
    typedef union DL_VERSIONTYPE
    {
        struct
        {
            DL_U8 nVersionMajor;   /**< Major version accessor element */
            DL_U8 nVersionMinor;   /**< Minor version accessor element */
            DL_U8 nRevision;       /**< Revision version accessor element */
            DL_U8 nStep;           /**< Step version accessor element */
        } s;
        DL_U32 nVersion;           /**<  bit value to make accessing the
                                      version easily done in a single word
                                      size copy/compare operation */
    } DL_VERSIONTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
