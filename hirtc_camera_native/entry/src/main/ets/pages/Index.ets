import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so';
import image from '@ohos.multimedia.image';
import abilityAccessCtrl from '@ohos.abilityAccessCtrl';
import common from '@ohos.app.ability.common'
import { Permissions } from '@ohos.abilityAccessCtrl'
import camera from '@ohos.multimedia.camera';

@Entry
@Component
struct Index {

  private dataSize: image.Size = {
    height: 1920,
    width: 1080
  };
  private x_width = 180;
  private y_height = 320;
  private receiver = image.createImageReceiver(this.dataSize, 2000, 8);
  private mXComponentController: XComponentController = new XComponentController;
  private XComponentSurfaceId: string = 'preview';
  private xComponentContext: object | undefined = undefined;

  build() {
    Column() {
      Row() {
        Column() {
          XComponent({
            id: this.XComponentSurfaceId,
            type: 'surface',
            libraryname: 'entry',
            controller: this.mXComponentController
          })
            .width(this.x_width)
            .height(this.y_height)
            .onLoad((xComponentContext) => {
              // 设置Surface宽高（1920*1080），预览尺寸设置参考前面 previewProfilesArray 获取的当前设备所支持的预览分辨率大小去设置
              // 预览流与录像输出流的分辨率的宽高比要保持一致
              // this.xComponentContext = xComponentContext;
              // this.mXComponentController.setXComponentSurfaceRect({offsetX: 0, offsetY: 0, surfaceWidth: 1080, surfaceHeight: 1920 });
              //this.mXComponentController.setXComponentSurfaceSize({ surfaceWidth: 360, surfaceHeight: 640 });
            })
        }.width('50%')
        Column(){
          XComponent({
            id: '12345',
            type: 'surface',
            libraryname: 'entry',
          })
            .width(this.x_width)
            .height(this.y_height)
            .onLoad((xComponentContext) => {
              // 设置Surface宽高（1920*1080），预览尺寸设置参考前面 previewProfilesArray 获取的当前设备所支持的预览分辨率大小去设置
              // 预览流与录像输出流的分辨率的宽高比要保持一致
              // this.xComponentContext = xComponentContext;
              // this.mXComponentController.setXComponentSurfaceRect({offsetX: 0, offsetY: 0, surfaceWidth: 1080, surfaceHeight: 1920 });
              //this.mXComponentController.setXComponentSurfaceSize({ surfaceWidth: 360, surfaceHeight: 640 });
            })
        }.width('50%')
      }.position({x:0, y:260})
      Row() {
        Column() {
          Button("start")
            .width(100)
            .height(100)
            .onClick(() => {
              abilityAccessCtrl.createAtManager()
                .requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA'])
                .then(async () => {
                  let receiverSurfaceId: string = await this.receiver.getReceivingSurfaceId();
                  this.XComponentSurfaceId = this.mXComponentController.getXComponentSurfaceId();
                  console.log("button click in");
                  testNapi.initCamera(receiverSurfaceId, this.XComponentSurfaceId);
                  console.log("add fun end");
                })
            })
        }
        .layoutWeight(1)
        Column() {
          Button("stop")
            .width(100)
            .height(100)
            .onClick(() => {
              abilityAccessCtrl.createAtManager()
                .requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA'])
                .then(async () => {
                  testNapi.stopCamera();
                })
            })
        }
        .layoutWeight(1)
        Column() {
          Button("反转")
            .width(100)
            .height(100)
            .onClick(() => {
              abilityAccessCtrl.createAtManager()
                .requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA'])
                .then(async () => {
                  testNapi.changeCamera();
                })
            })
        }
        .layoutWeight(1)
      }
      .position({x:0, y:20})
      .layoutWeight(100)
    }
    .height('100%')
  }
}