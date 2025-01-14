/*******************************************************************************
 * Copyright (c) 2013, Esoteric Software
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include <spine/SkeletonJson.h>
#include <math.h>
#include <stdio.h>
#include <math.h>
#include "Json.h"
#include <spine/extension.h>
#include <spine/RegionAttachment.h>
#include <spine/AtlasAttachmentLoader.h>

namespace cocos2d { namespace extension {

typedef struct {
	SkeletonJson super;
	int ownsLoader;
} _Internal;

SkeletonJson* SkeletonJson_createWithLoader (AttachmentLoader* attachmentLoader) {
	SkeletonJson* self = SUPER(NEW(_Internal));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

SkeletonJson* SkeletonJson_create (Atlas* atlas) {
	AtlasAttachmentLoader* attachmentLoader = AtlasAttachmentLoader_create(atlas);
	SkeletonJson* self = SkeletonJson_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_Internal, self) ->ownsLoader = 1;
	return self;
}

void SkeletonJson_dispose (SkeletonJson* self) {
	if (SUB_CAST(_Internal, self) ->ownsLoader) AttachmentLoader_dispose(self->attachmentLoader);
	FREE(self->error);
	FREE(self);
}

void _SkeletonJson_setError (SkeletonJson* self, Json* root, const char* value1, const char* value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = strlen(value1);
	if (value2) strncat(message + length, value2, 256 - length);
	MALLOC_STR(self->error, message);
	if (root) Json_dispose(root);
}

static float toColor (const char* value, int index) {
	char digits[3];
	char *error;
	int color;

	if (strlen(value) != 8) return -1;
	value += index * 2;
	
	digits[0] = *value;
	digits[1] = *(value + 1);
	digits[2] = '\0';
	color = strtoul(digits, &error, 16);
	if (*error != 0) return -1;
	return color / (float)255;
}

static void readCurve (CurveTimeline* timeline, int frameIndex, Json* frame) {
	Json* curve = Json_getItem(frame, "curve");
	if (!curve) return;
	if (curve->type == Json_String && strcmp(curve->valuestring, "stepped") == 0)
		CurveTimeline_setStepped(timeline, frameIndex);
	else if (curve->type == Json_Array) {
        Json* child0 = curve->child;
        Json* child1 = child0->next;
        Json* child2 = child1->next;
        Json* child3 = child2->next;
		CurveTimeline_setCurve(timeline, frameIndex, child0->valuefloat, child1->valuefloat, child2->valuefloat, child3->valuefloat);
	}
}

static Animation* _SkeletonJson_readAnimation (SkeletonJson* self, Json* root, SkeletonData *skeletonData) {
	Animation* animation;

	Json* bones = Json_getItem(root, "bones");
	int boneCount = bones ? Json_getSize(bones) : 0;

	Json* slots = Json_getItem(root, "slots");
	int slotCount = slots ? Json_getSize(slots) : 0;

	int timelineCount = 0;
	int i(0), ii(0), iii(0);
    Json *boneMap(NULL), *slotMap(NULL);

    for (boneMap = bones ? bones->child : 0; boneMap; boneMap = boneMap->next)
        timelineCount += boneMap->size;
    for (slotMap = slots ? slots->child : 0; slotMap; slotMap = slotMap->next)
        timelineCount += slotMap->size;

	animation = Animation_create(root->name, timelineCount);
	animation->timelineCount = 0;
	skeletonData->animations[skeletonData->animationCount] = animation;
	skeletonData->animationCount++;

    for (boneMap = bones ? bones->child : 0, i=0; boneMap; boneMap = boneMap->next, i++) {

		const char* boneName = boneMap->name;

		int boneIndex = SkeletonData_findBoneIndex(skeletonData, boneName);
		if (boneIndex == -1) {
			Animation_dispose(animation);
			_SkeletonJson_setError(self, root, "Bone not found: ", boneName);
			return 0;
		}

        Json* timelineArray(NULL);
        for (timelineArray = boneMap->child, ii=0; timelineArray; timelineArray = timelineArray->next, ii++) {
			float duration;
			int frameCount = Json_getSize(timelineArray);
			const char* timelineType = timelineArray->name;

			if (strcmp(timelineType, "rotate") == 0) {
				
				RotateTimeline *timeline = RotateTimeline_create(frameCount);
				timeline->boneIndex = boneIndex;

                Json* frame(NULL);
                for (frame = timelineArray->child, iii = 0; frame; frame = frame->next, iii++) {

					RotateTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0), Json_getFloat(frame, "angle", 0));
					readCurve(SUPER(timeline), iii, frame);
				}
				animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
				duration = timeline->frames[frameCount * 2 - 2];
				if (duration > animation->duration) animation->duration = duration;

			} else {
				int isScale = strcmp(timelineType, "scale") == 0;
				if (isScale || strcmp(timelineType, "translate") == 0) {
					float scale = isScale ? 1 : self->scale;
					TranslateTimeline *timeline = isScale ? ScaleTimeline_create(frameCount) : TranslateTimeline_create(frameCount);
					timeline->boneIndex = boneIndex;

                    Json* frame (NULL);
                    for (frame = timelineArray->child, iii = 0; frame; frame = frame->next, iii++) {

						TranslateTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0), Json_getFloat(frame, "x", 0) * scale,
								Json_getFloat(frame, "y", 0) * scale);
						readCurve(SUPER(timeline), iii, frame);
					}
					animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
					duration = timeline->frames[frameCount * 3 - 3];
					if (duration > animation->duration) animation->duration = duration;
				} else {
					Animation_dispose(animation);
					_SkeletonJson_setError(self, 0, "Invalid timeline type for a bone: ", timelineType);
					return 0;
				}
			}
		}
	}

    for (slotMap = slots ? slots->child : 0, i=0; slotMap; slotMap = slotMap->next, i++) {

		const char* slotName = slotMap->name;

		int slotIndex = SkeletonData_findSlotIndex(skeletonData, slotName);
		if (slotIndex == -1) {
			Animation_dispose(animation);
			_SkeletonJson_setError(self, root, "Slot not found: ", slotName);
			return 0;
		}

        Json* timelineArray(NULL);
        for (timelineArray = slotMap->child, ii=0; timelineArray; timelineArray = timelineArray->next, ii++) {
			float duration;

			int frameCount = Json_getSize(timelineArray);
			const char* timelineType = timelineArray->name;

			if (strcmp(timelineType, "color") == 0) {
				ColorTimeline *timeline = ColorTimeline_create(frameCount);
				timeline->slotIndex = slotIndex;

                Json* frame(NULL);
                for (frame = timelineArray->child, iii = 0; frame; frame = frame->next, iii++) {

					const char* s = Json_getString(frame, "color", 0);
					ColorTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0), toColor(s, 0), toColor(s, 1), toColor(s, 2),
							toColor(s, 3));
					readCurve(SUPER(timeline), iii, frame);
				}
				animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
				duration = timeline->frames[frameCount * 5 - 5];
				if (duration > animation->duration) animation->duration = duration;

			} else if (strcmp(timelineType, "attachment") == 0) {

				AttachmentTimeline *timeline = AttachmentTimeline_create(frameCount);
				timeline->slotIndex = slotIndex;

                Json* frame(NULL);
                for (frame = timelineArray->child, iii = 0; frame; frame = frame->next, iii++) {

					Json* name = Json_getItem(frame, "name");
					AttachmentTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0),
							name->type == Json_NULL ? 0 : name->valuestring);
				}
				animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
				duration = timeline->frames[frameCount - 1];
				if (duration > animation->duration) animation->duration = duration;

			} else {
				Animation_dispose(animation);
				_SkeletonJson_setError(self, 0, "Invalid timeline type for a slot: ", timelineType);
				return 0;
			}
		}
	}

	return animation;
}

SkeletonData* SkeletonJson_readSkeletonDataFile (SkeletonJson* self, const char* path) {
	int length;
	SkeletonData* skeletonData;
	const char* json = _Util_readFile(path, &length);
	if (!json) {
		_SkeletonJson_setError(self, 0, "Unable to read skeleton file: ", path);
		return 0;
	}
	skeletonData = SkeletonJson_readSkeletonData(self, json);
	FREE(json);
	return skeletonData;
}

SkeletonData* SkeletonJson_readSkeletonData (SkeletonJson* self, const char* json) {
	SkeletonData* skeletonData;
	Json *root, *bones;
	int i(0), ii(0), iii(0), boneCount(0);
	Json* slots(NULL);
	Json* skinsMap(NULL);
	Json* animations(NULL);

	FREE(self->error);
	CONST_CAST(char*, self->error) = 0;

    JsonAllocator allocator;
	root = Json_create(json, allocator);
	if (!root) {
		_SkeletonJson_setError(self, 0, "Invalid skeleton JSON: ", Json_getError());
		return 0;
	}

	skeletonData = SkeletonData_create();

	bones = Json_getItem(root, "bones");
	boneCount = Json_getSize(bones);
	skeletonData->bones = MALLOC(BoneData*, boneCount);
    Json* boneMap(NULL);
    for (boneMap = bones->child; boneMap; boneMap = boneMap->next, i++)
    {
		BoneData* boneData;

		const char* boneName = Json_getString(boneMap, "name", 0);

		BoneData* parent = 0;
		const char* parentName = Json_getString(boneMap, "parent", 0);
		if (parentName) {
			parent = SkeletonData_findBone(skeletonData, parentName);
			if (!parent) {
				SkeletonData_dispose(skeletonData);
				_SkeletonJson_setError(self, root, "Parent bone not found: ", parentName);
				return 0;
			}
		}

		boneData = BoneData_create(boneName, parent);
		boneData->length = Json_getFloat(boneMap, "length", 0) * self->scale;
		boneData->x = Json_getFloat(boneMap, "x", 0) * self->scale;
		boneData->y = Json_getFloat(boneMap, "y", 0) * self->scale;
		boneData->rotation = Json_getFloat(boneMap, "rotation", 0);
		boneData->scaleX = Json_getFloat(boneMap, "scaleX", 1);
		boneData->scaleY = Json_getFloat(boneMap, "scaleY", 1);

		skeletonData->bones[i] = boneData;
		skeletonData->boneCount++;
	}

	slots = Json_getItem(root, "slots");
	if (slots) {
		int slotCount = Json_getSize(slots);
		skeletonData->slots = MALLOC(SlotData*, slotCount);

        Json* slotMap(NULL);
        for (slotMap = slots->child, i=0; slotMap; slotMap = slotMap->next, i++)
        {
			SlotData* slotData;
			const char* color;
			Json *attachmentItem;


			const char* slotName = Json_getString(slotMap, "name", 0);

			const char* boneName = Json_getString(slotMap, "bone", 0);
			BoneData* boneData = SkeletonData_findBone(skeletonData, boneName);
			if (!boneData) {
				SkeletonData_dispose(skeletonData);
				_SkeletonJson_setError(self, root, "Slot bone not found: ", boneName);
				return 0;
			}

			slotData = SlotData_create(slotName, boneData);

			color = Json_getString(slotMap, "color", 0);
			if (color) {
				slotData->r = toColor(color, 0);
				slotData->g = toColor(color, 1);
				slotData->b = toColor(color, 2);
				slotData->a = toColor(color, 3);
			}

			attachmentItem = Json_getItem(slotMap, "attachment");
			if (attachmentItem) SlotData_setAttachmentName(slotData, attachmentItem->valuestring);

			skeletonData->slots[i] = slotData;
			skeletonData->slotCount++;
		}
	}

	skinsMap = Json_getItem(root, "skins");
	if (skinsMap) {
		int skinCount = Json_getSize(skinsMap);
		skeletonData->skins = MALLOC(Skin*, skinCount);

        Json* slotMap(NULL);
        for (slotMap = skinsMap->child, i=0; slotMap; slotMap = slotMap->next, i++) {

			const char* skinName = slotMap->name;
			Skin *skin = Skin_create(skinName);

			skeletonData->skins[i] = skin;
			skeletonData->skinCount++;
			if (strcmp(skinName, "default") == 0) skeletonData->defaultSkin = skin;

            Json* attachmentsMap(NULL);
            for (attachmentsMap = slotMap->child, ii=0; attachmentsMap; attachmentsMap = attachmentsMap->next, ii++) {

				const char* slotName = attachmentsMap->name;
				int slotIndex = SkeletonData_findSlotIndex(skeletonData, slotName);

                iii = 0;
                Json* attachmentMap(NULL);
                for (attachmentMap = attachmentsMap->child, iii=0; attachmentMap; attachmentMap = attachmentMap->next, iii++) {

					Attachment* attachment;
					const char* skinAttachmentName = attachmentMap->name;
					const char* attachmentName = Json_getString(attachmentMap, "name", skinAttachmentName);

					const char* typeString = Json_getString(attachmentMap, "type", "region");
					AttachmentType type;
					if (strcmp(typeString, "region") == 0)
						type = ATTACHMENT_REGION;
					else if (strcmp(typeString, "regionSequence") == 0)
						type = ATTACHMENT_REGION_SEQUENCE;
					else {
						SkeletonData_dispose(skeletonData);
						_SkeletonJson_setError(self, root, "Unknown attachment type: ", typeString);
						return 0;
					}

					attachment = AttachmentLoader_newAttachment(self->attachmentLoader, skin, type, attachmentName);
					if (!attachment) {
						if (self->attachmentLoader->error1) {
							SkeletonData_dispose(skeletonData);
							_SkeletonJson_setError(self, root, self->attachmentLoader->error1, self->attachmentLoader->error2);
							return 0;
						}
						continue;
					}

					if (attachment->type == ATTACHMENT_REGION || attachment->type == ATTACHMENT_REGION_SEQUENCE) {
						RegionAttachment* regionAttachment = (RegionAttachment*)attachment;
						regionAttachment->x = Json_getFloat(attachmentMap, "x", 0) * self->scale;
						regionAttachment->y = Json_getFloat(attachmentMap, "y", 0) * self->scale;
						regionAttachment->scaleX = Json_getFloat(attachmentMap, "scaleX", 1);
						regionAttachment->scaleY = Json_getFloat(attachmentMap, "scaleY", 1);
						regionAttachment->rotation = Json_getFloat(attachmentMap, "rotation", 0);
						regionAttachment->width = Json_getFloat(attachmentMap, "width", 32) * self->scale;
						regionAttachment->height = Json_getFloat(attachmentMap, "height", 32) * self->scale;
						RegionAttachment_updateOffset(regionAttachment);
					}

					Skin_addAttachment(skin, slotIndex, skinAttachmentName, attachment);
				}
			}
		}
	}

	animations = Json_getItem(root, "animations");
	if (animations) {
		int animationCount = Json_getSize(animations);
		skeletonData->animations = MALLOC(Animation*, animationCount);
        Json* animationMap(NULL);
        for (animationMap = animations->child; animationMap; animationMap = animationMap->next)
        {
			_SkeletonJson_readAnimation(self, animationMap, skeletonData);
		}
	}

	Json_dispose(root);
	return skeletonData;
}

}} // namespace cocos2d { namespace extension {