/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <binder/Binder.h>
#include <binder/Parcel.h>

#include <gui/LayerState.h>

namespace android {
namespace test {

TEST(LayerStateTest, ParcellingDisplayCaptureArgs) {
    DisplayCaptureArgs args;
    args.pixelFormat = ui::PixelFormat::RGB_565;
    args.sourceCrop = Rect(0, 0, 500, 200);
    args.frameScale = 2;
    args.captureSecureLayers = true;
    args.displayToken = new BBinder();
    args.width = 10;
    args.height = 20;
    args.useIdentityTransform = true;

    Parcel p;
    args.write(p);
    p.setDataPosition(0);

    DisplayCaptureArgs args2;
    args2.read(p);

    ASSERT_EQ(args.pixelFormat, args2.pixelFormat);
    ASSERT_EQ(args.sourceCrop, args2.sourceCrop);
    ASSERT_EQ(args.frameScale, args2.frameScale);
    ASSERT_EQ(args.captureSecureLayers, args2.captureSecureLayers);
    ASSERT_EQ(args.displayToken, args2.displayToken);
    ASSERT_EQ(args.width, args2.width);
    ASSERT_EQ(args.height, args2.height);
    ASSERT_EQ(args.useIdentityTransform, args2.useIdentityTransform);
}

TEST(LayerStateTest, ParcellingLayerCaptureArgs) {
    LayerCaptureArgs args;
    args.pixelFormat = ui::PixelFormat::RGB_565;
    args.sourceCrop = Rect(0, 0, 500, 200);
    args.frameScale = 2;
    args.captureSecureLayers = true;
    args.layerHandle = new BBinder();
    args.excludeHandles = {new BBinder(), new BBinder()};
    args.childrenOnly = false;

    Parcel p;
    args.write(p);
    p.setDataPosition(0);

    LayerCaptureArgs args2;
    args2.read(p);

    ASSERT_EQ(args.pixelFormat, args2.pixelFormat);
    ASSERT_EQ(args.sourceCrop, args2.sourceCrop);
    ASSERT_EQ(args.frameScale, args2.frameScale);
    ASSERT_EQ(args.captureSecureLayers, args2.captureSecureLayers);
    ASSERT_EQ(args.layerHandle, args2.layerHandle);
    ASSERT_EQ(args.excludeHandles, args2.excludeHandles);
    ASSERT_EQ(args.childrenOnly, args2.childrenOnly);
}

TEST(LayerStateTest, ParcellingScreenCaptureResults) {
    ScreenCaptureResults results;
    results.buffer = new GraphicBuffer(100, 200, PIXEL_FORMAT_RGBA_8888, 1, 0);
    results.capturedSecureLayers = true;
    results.capturedDataspace = ui::Dataspace::DISPLAY_P3;
    results.result = BAD_VALUE;

    Parcel p;
    results.write(p);
    p.setDataPosition(0);

    ScreenCaptureResults results2;
    results2.read(p);

    // GraphicBuffer object is reallocated so compare the data in the graphic buffer
    // rather than the object itself
    ASSERT_EQ(results.buffer->getWidth(), results2.buffer->getWidth());
    ASSERT_EQ(results.buffer->getHeight(), results2.buffer->getHeight());
    ASSERT_EQ(results.buffer->getPixelFormat(), results2.buffer->getPixelFormat());
    ASSERT_EQ(results.capturedSecureLayers, results2.capturedSecureLayers);
    ASSERT_EQ(results.capturedDataspace, results2.capturedDataspace);
    ASSERT_EQ(results.result, results2.result);
}

} // namespace test
} // namespace android
