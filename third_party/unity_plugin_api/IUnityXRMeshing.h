// Unity Native Plugin API copyright © 2019 Unity Technologies ApS
//
// Licensed under the Unity Companion License for Unity - dependent projects--see [Unity Companion License](http://www.unity3d.com/legal/licenses/Unity_Companion_License).
//
// Unless expressly provided otherwise, the Software under this license is made available strictly on an “AS IS” BASIS WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.Please review the license for details on these and other terms and conditions.

#pragma once
#if UNITY
#   include "Modules/XR/ProviderInterface/UnityXRTypes.h"
#   include "Modules/XR/ProviderInterface/UnityXRSubsystemTypes.h"
#else
#   include "UnityXRTypes.h"
#   include "UnityXRSubsystemTypes.h"
#endif

#include <stddef.h>
#include <stdint.h>

/// @file IUnityXRMeshing.h
/// @brief XR interface for mesh generation and allocation of related data structures.
/// @see IUnityXRMeshInterface

/// The format used for an index buffer
typedef enum UnityXRIndexFormat
{
    kUnityXRIndexFormat16Bit,   ///< 16 bit indices, e.g., uint16_t
    kUnityXRIndexFormat32Bit    ///< 32 bit indices, e.g., uint32_t
} UnityXRIndexFormat;

/// The type of mesh data, e.g., points, triangles, quads, etc.
/// This affects how the vertex and index data are interpreted.
typedef enum UnityXRMeshTopology
{
    /// The mesh data represents triangles, with 3 indices per triangle.
    kUnityXRMeshTopologyTriangles,

    /// The mesh data represents a triangle strip. There are 3 indices
    /// for the first triangle, then every point after that is another triangle.
    kUnityXRMeshTopologyTriangleStrip,

    /// The mesh data represents quads, with 4 indices per quad.
    kUnityXRMeshTopologyQuads,

    /// The mesh data represents lines, with 2 indices per line.
    kUnityXRMeshTopologyLines,

    /// The mesh data represents a line strip. The are 2 indices
    /// for the first line, then every index after that is another line.
    kUnityXRMeshTopologyLineStrip,

    /// The mesh data represents individual points, with 1 index per point.
    kUnityXRMeshTopologyPoints
} UnityXRMeshTopology;

/// Flags representing vertex attributes
typedef enum UnityXRMeshVertexAttributeFlags
{
    /// The vertex contains normal data
    kUnityXRMeshVertexAttributeFlagsNormals = 1 << 0,

    /// The vertex contains tangent data
    kUnityXRMeshVertexAttributeFlagsTangents = 1 << 1,

    /// The vertex contains uv data
    kUnityXRMeshVertexAttributeFlagsUvs = 1 << 2,

    /// The vertex contains color data
    kUnityXRMeshVertexAttributeFlagsColors = 1 << 3
} UnityXRMeshVertexAttributeFlags;

/// Describes a single mesh, with buffers pointing to each vertex attribute.
/// Note: Unity uses a left-handed coordinate system with clockwise triangle winding.
/// +X is right
/// +Y is up
/// +Z is forward
typedef struct UnityXRMeshDescriptor
{
    /// Pointer to positions buffer. May not be null.
    UnityXRVector3* positions;

    /// Pointer to normals buffer. May be null.
    UnityXRVector3* normals;

    /// Pointer to tangents buffer. May be null.
    UnityXRVector4* tangents;

    /// Pointer to uvs buffer. May be null.
    UnityXRVector2* uvs;

    /// Pointer to colors buffer. May be null.
    UnityXRColorRGBA32* colors;

    /// Index buffers
    union
    {
        uint16_t* indices16; ///< Used if @a indexFormat is @a kUnityXRIndexFormat16Bit
        uint32_t* indices32; ///< Used if @a indexFormat is @a kUnityXRIndexFormat32Bit
    };

    /// The number of vertices. Each non-null buffer above must
    /// contain this number of elements.
    size_t vertexCount;

    /// The number of indices in the mesh.
    size_t indexCount;

    /// Which index format should be used, 16 or 32 bit.
    UnityXRIndexFormat indexFormat;

    /// The primitive type of the mesh data (e.g., points, lines, triangles, quads)
    UnityXRMeshTopology topology;
} UnityXRMeshDescriptor;

/// Interface for allocating or setting mesh-related data
typedef struct UnityXRMeshDataAllocator UnityXRMeshDataAllocator;

/// A unique id assignable to a 'trackable', e.g. a plane or
/// point cloud point. Although this id is 128 bits, it may
/// not necessarily be globally unique; it just needs to be
/// unique to a particular XREnvironment session.
typedef struct UnityXRMeshId
{
    /// 128-bit id data
    uint64_t idPart[2];
} UnityXRMeshId;

#ifdef __cplusplus
inline bool operator==(const UnityXRMeshId& a, const UnityXRMeshId& b)
{
    return
        (a.idPart[0] == b.idPart[0]) &&
        (a.idPart[1] == b.idPart[1]);
}

inline bool operator!=(const UnityXRMeshId& a, const UnityXRMeshId& b)
{
    return
        (a.idPart[0] != b.idPart[0]) ||
        (a.idPart[1] != b.idPart[1]);
}

/// A comparator for UnityXRMeshId suitable for algorithms or containers that
/// require ordered UnityXRMeshIds, e.g. std::map
struct MeshIdLessThanComparator
{
    bool operator()(const UnityXRMeshId& lhs, const UnityXRMeshId& rhs) const
    {
        if (lhs.idPart[0] == rhs.idPart[0])
        {
            return lhs.idPart[1] < rhs.idPart[1];
        }

        return lhs.idPart[0] < rhs.idPart[0];
    }
};
#endif // __cplusplus

/// Information related to a mesh
typedef struct UnityXRMeshInfo
{
    /// A session-unique identifier for the mesh.
    UnityXRMeshId meshId;

    /// True if the mesh has been updated (e.g., since the last query).
    bool updated;

    /// A hint for the system that determines when it should update
    /// the corresponding Unity mesh.
    int priorityHint;
} UnityXRMeshInfo;

/// An allocator for the @a UnityXRMeshInfo type.
typedef struct UnityXRMeshInfoAllocator UnityXRMeshInfoAllocator;

/// Enum describing the type of the bounding volume that will be passed
/// into @a SetBoundingVolume.
typedef enum UnityXRBoundingVolumeType
{
    /// Axis Aligned Bounding Box bounding volume
    kUnityXRBoundingVolumeAABB,
} UnityXRBoundingVolumeType;

/// Definition for an Axis Aligned Bounding Box (AABB).
/// This type of volume is aligned with the axis of the
/// current coordinate space. The definition of the
/// dimensions (m, cm, ft, etc.) is left up to the provider.
typedef struct UnityXRAABBBoundingVolume
{
    /// The origin or center point for the AABB
    /// In the current coordinate space.
    UnityXRVector3 origin;

    /// The extents of the AABB in each major axis
    /// in the current coordinate space.
    UnityXRVector3 extents;
} UnityXRAABBBoundingVolume;

/// Generic definition for a bounding volume passed to the
/// @a SetBoundingVolume api.
typedef struct UnityXRBoundingVolume
{
    /// The type of the bounding volume described by
    /// an instance of this struct.
    /// See @a UnityXRBoundingVolumeType
    UnityXRBoundingVolumeType type;

    /// The actual bounding volume definition for
    /// an instance of this struct. The specific field
    /// to be used will be determined by what is set in the
    /// @a type field.
    union
    {
        /// See @a UnityXRAABBBoundingVolume
        UnityXRAABBBoundingVolume aabb;
    };
} UnityXRBoundingVolume;

/// Implement this interface for your mesh provider. A mesh provider will typically
/// provide multiple meshes, addressable by @a UnityXRMeshId.
typedef struct SUBSYSTEM_PROVIDER UnityXRMeshProvider
{
    /// Plugin-specific data pointer. Every function in this structure will
    /// be passed this data by the Unity runtime. The plugin should allocate
    /// an appropriate data container for any information that is needed when
    /// a callback function is called.
    void* userData;

    /// Invoked by Unity to retrieve information about every tracked mesh.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param userData Value of userData field when provider was registered.
    /// @param allocator Use this allocator to get a buffer of @a UnityXRMeshInfo.
    /// This method should populate the buffer.
    /// @return A @a UnitySubsystemErrorCode indicating success or failure.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * GetMeshInfos)(
        UnitySubsystemHandle handle, void* userData, UnityXRMeshInfoAllocator * allocator);

    /// Invoked from any thread by Unity to acquire a particular mesh by id.
    /// This method should fill out a @a UnityXRMeshDescriptor using the provided
    /// @a allocator. If the plugin owns the mesh memory, it should not mutate
    /// the data until @a ReleaseMesh is called (see below).
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param userData Value of userData field when provider was registered.
    /// @param meshId The session-unique identifier associated with this mesh.
    /// @param allocator Used to either allocate a @a UnityXRMeshDescriptor from Unity
    /// or provide one owned by the plugin.
    /// @return A @a UnitySubsystemErrorCode indicating success or failure.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * AcquireMesh)(
        UnitySubsystemHandle handle,
        void* userData,
        const UnityXRMeshId * meshId,
        UnityXRMeshDataAllocator * allocator);

    /// Invoked by Unity on the main thread to release a mesh by id.
    /// This method should perform any required cleanup for the mesh.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param userData Value of userData field when provider was registered.
    /// @param meshId The session-unique identifier associated with this mesh.
    /// @param mesh If the plugin owns the memory, then this is the same mesh
    /// descriptor provided by the plugin in @a AcquireMesh. Otherwise, if Unity
    /// owns the memory, then @a mesh will be null.
    /// @param pluginData A pointer provided by the provider during @a AcquireMesh.
    /// @return A @a UnitySubsystemErrorCode indicating success or failure.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * ReleaseMesh)(
        UnitySubsystemHandle handle,
        void* userData,
        const UnityXRMeshId * meshId,
        const UnityXRMeshDescriptor * mesh,
        void* pluginData);

    /// Invoked by Unity on the main thread to request a change in the density
    /// of the generated meshes. Density level will be given as a value in the
    /// range [0.0-1.0] and the provider will determine how best to map that
    /// value to their implementation.
    ///
    /// Setting this value does not guarantee an immediate change in the density
    /// of any currently created mesh and may only change the density for new or
    /// updated meshes.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param userData Value of userData field when provider was registered.
    /// @param density The requested density of generated meshes. Defined as a
    ///                float in the range [0.0-1.0]
    /// @return A @a UnitySubsystemErrorCode indicating success or failure.
    /// @a kUnitySubsystemErrorCodeSuccess Returned if everything is ok.
    /// @a kUnitySubsystemErrorCodeInvalidArguments Returned if the density level
    ///    falls outside the expected range.
    /// @a kUnitySubsystemErrorCodeNotSupported Returned if the provider does not
    ///    support this API in their implementation.
    /// @a kUnitySubsystemErrorCodeFailure Returned for ny other error or issue
    ///    not defined above.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SetMeshDensity)(
        UnitySubsystemHandle handle,
        void* userData,
        float density);


    /// Set the bounding volume to be used to restrict the space in which meshes will
    /// be generated and tracked.
    ///
    /// @param handle Handle obtained from UnityLifecycleProvider callbacks.
    /// @param userData Value of userData field when provider was registered.
    /// @param boundingVolume The bounding volume definition to use.
    /// @return A @a UnitySubsystemErrorCode indicating success or failure.
    /// @a kUnitySubsystemErrorCodeSuccess Returned if everything is ok.
    /// @a kUnitySubsystemErrorCodeInvalidArguments Returned it the bounding
    ///    volume is null or the definition is incorrect.
    /// @a kUnitySubsystemErrorCodeNotSupported Returned if the provider does not
    ///    support this API.
    /// @a kUnitySubsystemErrorCodeNotSupported Returned if the provider does not
    ///    support the type of bounding volume being requested.
    /// @a kUnitySubsystemErrorCodeFailure Returned for ny other error or issue
    ///    not defined above.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SetBoundingVolume)(
        UnitySubsystemHandle handle,
        void* userData,
        const UnityXRBoundingVolume* boundingVolume);
} UnityXRMeshProvider;


/// @brief XR interface for mesh generation and allocation of related data structures.
UNITY_DECLARE_INTERFACE(SUBSYSTEM_INTERFACE IUnityXRMeshInterface)
{
    /// Allocates @a count @a UnityXRMeshInfo. Unity will periodically invoke your
    /// plugin's @a GetMeshInfos function. Your plugin should then call
    /// this function to allocate an array of @a UnityXRMeshInfo and populate them.
    ///
    /// @param count The number of meshes currently tracked.
    /// @return An array of @a UnityXRMeshInfo which the caller should populate with mesh info.
    UnityXRMeshInfo* (UNITY_INTERFACE_API * MeshInfoAllocator_Allocate)(
        UnityXRMeshInfoAllocator * allocator, size_t count);

    /// Allocates mesh data according to the attributes specified in @a attributes.
    /// Use this method if you want Unity to manage the mesh memory.
    ///
    /// @param vertexCount The number of vertices to allocate.
    /// @param triangleCount The number of triangles to allocate.
    /// @param indexFormat The @a UnityXRIndexFormat indicating 16 or 32 bit indices
    /// @param attributes The vertex attributes for which to allocate buffers.
    UnityXRMeshDescriptor* (UNITY_INTERFACE_API * MeshDataAllocator_AllocateMesh)(
        UnityXRMeshDataAllocator * allocator,
        size_t vertexCount,
        size_t indexCount,
        UnityXRIndexFormat indexFormat,
        UnityXRMeshVertexAttributeFlags attributes,
        UnityXRMeshTopology topology);

    /// Sets the mesh descriptor to use. Use this method if the plugin manages the memory.
    /// The buffers pointed to by @a mesh must exist until Unity invokes @a ReleaseMesh.
    ///
    /// @param meshDescriptor The mesh descriptor for a mesh. Unused vertex attributes should
    /// be null. @a positions must be non null.
    void(UNITY_INTERFACE_API * MeshDataAllocator_SetMesh)(
        UnityXRMeshDataAllocator * allocator, const UnityXRMeshDescriptor * meshDescriptor);

    /// (Optional) Set user data associated with this mesh. The userData pointer
    /// will be passed back to the provider's @a ReleaseMesh method.
    /// Use this to store additional data required to release the mesh.
    ///
    /// @param userData A pointer which will be passed to ReleaseMesh
    void(UNITY_INTERFACE_API * MeshDataAllocator_SetUserData)(
        UnityXRMeshDataAllocator * allocator, void* userData);

    /// Registers a mesh provider for your plugin to communicate mesh data back to Unity.
    /// @param meshProvider The @a UnityXRMeshProvider which will be invoked by Unity to
    /// retrieve mesh data.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * SUBSYSTEM_REGISTER_PROVIDER(UnityXRMeshProvider) RegisterMeshProvider)(
        UnitySubsystemHandle handle, const UnityXRMeshProvider * meshProvider);

    /// Entry-point for getting callbacks when the mesh subsystem is initialized / started / stopped / shutdown.
    ///
    /// @param[in] pluginName Name of the plugin which was registered in your xr.json
    /// @param[in] id Id of the subsystem that was registered in your xr.json
    /// @param[in] provider Callbacks to register.
    /// @return Status of function execution.
    UnitySubsystemErrorCode(UNITY_INTERFACE_API * RegisterLifecycleProvider)(
        const char* pluginName, const char* id, const UnityLifecycleProvider * provider);
};

UNITY_REGISTER_INTERFACE_GUID(0x3007fd5885a346efULL, 0x9eeb2c84aa0a9dd9ULL, IUnityXRMeshInterface)
