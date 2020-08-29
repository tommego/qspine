TEMPLATE = lib

TARGET = spine-cpp

CONFIG -= lib_bundle
CONFIG -= qt
CONFIG += staticlib

INCLUDEPATH += \
    $$PWD/include

HEADERS += \
    include/spine/Animation.h \
    include/spine/AnimationState.h \
    include/spine/AnimationStateData.h \
    include/spine/Atlas.h \
    include/spine/AtlasAttachmentLoader.h \
    include/spine/Attachment.h \
    include/spine/AttachmentLoader.h \
    include/spine/AttachmentTimeline.h \
    include/spine/AttachmentType.h \
    include/spine/BlendMode.h \
    include/spine/Bone.h \
    include/spine/BoneData.h \
    include/spine/BoundingBoxAttachment.h \
    include/spine/ClippingAttachment.h \
    include/spine/Color.h \
    include/spine/ColorTimeline.h \
    include/spine/ConstraintData.h \
    include/spine/ContainerUtil.h \
    include/spine/CurveTimeline.h \
    include/spine/Debug.h \
    include/spine/DeformTimeline.h \
    include/spine/DrawOrderTimeline.h \
    include/spine/Event.h \
    include/spine/EventData.h \
    include/spine/EventTimeline.h \
    include/spine/Extension.h \
    include/spine/HasRendererObject.h \
    include/spine/HashMap.h \
    include/spine/IkConstraint.h \
    include/spine/IkConstraintData.h \
    include/spine/IkConstraintTimeline.h \
    include/spine/Json.h \
    include/spine/LinkedMesh.h \
    include/spine/MathUtil.h \
    include/spine/MeshAttachment.h \
    include/spine/MixBlend.h \
    include/spine/MixDirection.h \
    include/spine/PathAttachment.h \
    include/spine/PathConstraint.h \
    include/spine/PathConstraintData.h \
    include/spine/PathConstraintMixTimeline.h \
    include/spine/PathConstraintPositionTimeline.h \
    include/spine/PathConstraintSpacingTimeline.h \
    include/spine/PointAttachment.h \
    include/spine/Pool.h \
    include/spine/PositionMode.h \
    include/spine/RTTI.h \
    include/spine/RegionAttachment.h \
    include/spine/RotateMode.h \
    include/spine/RotateTimeline.h \
    include/spine/ScaleTimeline.h \
    include/spine/ShearTimeline.h \
    include/spine/Skeleton.h \
    include/spine/SkeletonBinary.h \
    include/spine/SkeletonBounds.h \
    include/spine/SkeletonClipping.h \
    include/spine/SkeletonData.h \
    include/spine/SkeletonJson.h \
    include/spine/Skin.h \
    include/spine/Slot.h \
    include/spine/SlotData.h \
    include/spine/SpacingMode.h \
    include/spine/SpineObject.h \
    include/spine/SpineString.h \
    include/spine/TextureLoader.h \
    include/spine/Timeline.h \
    include/spine/TimelineType.h \
    include/spine/TransformConstraint.h \
    include/spine/TransformConstraintData.h \
    include/spine/TransformConstraintTimeline.h \
    include/spine/TransformMode.h \
    include/spine/TranslateTimeline.h \
    include/spine/Triangulator.h \
    include/spine/TwoColorTimeline.h \
    include/spine/Updatable.h \
    include/spine/Vector.h \
    include/spine/VertexAttachment.h \
    include/spine/VertexEffect.h \
    include/spine/Vertices.h \
    include/spine/dll.h \
    include/spine/spine.h

SOURCES += \
    src/spine/Animation.cpp \
    src/spine/AnimationState.cpp \
    src/spine/AnimationStateData.cpp \
    src/spine/Atlas.cpp \
    src/spine/AtlasAttachmentLoader.cpp \
    src/spine/Attachment.cpp \
    src/spine/AttachmentLoader.cpp \
    src/spine/AttachmentTimeline.cpp \
    src/spine/Bone.cpp \
    src/spine/BoneData.cpp \
    src/spine/BoundingBoxAttachment.cpp \
    src/spine/ClippingAttachment.cpp \
    src/spine/ColorTimeline.cpp \
    src/spine/ConstraintData.cpp \
    src/spine/CurveTimeline.cpp \
    src/spine/DeformTimeline.cpp \
    src/spine/DrawOrderTimeline.cpp \
    src/spine/Event.cpp \
    src/spine/EventData.cpp \
    src/spine/EventTimeline.cpp \
    src/spine/Extension.cpp \
    src/spine/IkConstraint.cpp \
    src/spine/IkConstraintData.cpp \
    src/spine/IkConstraintTimeline.cpp \
    src/spine/Json.cpp \
    src/spine/LinkedMesh.cpp \
    src/spine/MathUtil.cpp \
    src/spine/MeshAttachment.cpp \
    src/spine/PathAttachment.cpp \
    src/spine/PathConstraint.cpp \
    src/spine/PathConstraintData.cpp \
    src/spine/PathConstraintMixTimeline.cpp \
    src/spine/PathConstraintPositionTimeline.cpp \
    src/spine/PathConstraintSpacingTimeline.cpp \
    src/spine/PointAttachment.cpp \
    src/spine/RTTI.cpp \
    src/spine/RegionAttachment.cpp \
    src/spine/RotateTimeline.cpp \
    src/spine/ScaleTimeline.cpp \
    src/spine/ShearTimeline.cpp \
    src/spine/Skeleton.cpp \
    src/spine/SkeletonBinary.cpp \
    src/spine/SkeletonBounds.cpp \
    src/spine/SkeletonClipping.cpp \
    src/spine/SkeletonData.cpp \
    src/spine/SkeletonJson.cpp \
    src/spine/Skin.cpp \
    src/spine/Slot.cpp \
    src/spine/SlotData.cpp \
    src/spine/SpineObject.cpp \
    src/spine/TextureLoader.cpp \
    src/spine/Timeline.cpp \
    src/spine/TransformConstraint.cpp \
    src/spine/TransformConstraintData.cpp \
    src/spine/TransformConstraintTimeline.cpp \
    src/spine/TranslateTimeline.cpp \
    src/spine/Triangulator.cpp \
    src/spine/TwoColorTimeline.cpp \
    src/spine/Updatable.cpp \
    src/spine/VertexAttachment.cpp \
    src/spine/VertexEffect.cpp
