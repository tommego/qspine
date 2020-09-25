import QtQuick 2.12
import QtQuick.Window 2.12
import Beelab.SpineItem 1.0
import QtQuick.Controls 1.4
import Aimy.AimyQml 1.0

Window {
    visible: true
    width: 1000
    height: 700
    title: qsTr("Hello World")
    color: "#3fe"

    function updateBlendCcolor() {
        mySpine.blendColor = Qt.rgba(rSlider.value / 255.0, gSlider.value / 255.0, bSlider.value / 255.0, aSlider.value / 255.0);
    }

    SpineItem{
        id: mySpine
//        atlasFile: "file:///home/beetom/Desktop/e5+/blood_bar/Blood_bar_anination.atlas"
//        skeletonFile: "file:///home/beetom/Desktop/e5+/blood_bar/Blood_bar_anination.json"
        atlasFile: "examples/raptor/export/raptor-pma.atlas"
        skeletonFile: "examples/raptor/export/raptor-pro.json"
//        atlasFile: "examples/goblins/export/goblins-pma.atlas"
//        skeletonFile: "examples/goblins/export/goblins-ess.json"
//        atlasFile: "file:///home/beetom/Desktop/light/1.atlas"
//        skeletonFile: "file:///home/beetom/Desktop/light/1.json"
//        atlasFile: "file:///home/beetom/Desktop/e5+/top_bar/1.atlas"
//        skeletonFile: "file:///home/beetom/Desktop/e5+/top_bar/1.json"
        anchors.centerIn: parent
        fps: fpsSlider.value
        skeletonScale: scaleSlider.value
        timeScale: timeScaleSlider.value
        defaultMix: defautMixSlider.value
        blendColorChannel: 4
        onResourceReady: {
            animationRep.model = animations
            skinRep.model = skins
            setAnimation(0, "flip", true)
        }
    }

    Column{
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        Slider{
            id: rSlider
            width: 300
            minimumValue: 0
            maximumValue: 255
            stepSize: 1
            value: 255
            onValueChanged: updateBlendCcolor()
        }
        Slider{
            id: gSlider
            width: 300
            minimumValue: 0
            maximumValue: 255
            stepSize: 1
            value: 255
            onValueChanged: updateBlendCcolor()
        }
        Slider{
            id: bSlider
            width: 300
            minimumValue: 0
            maximumValue: 255
            stepSize: 1
            value: 255
            onValueChanged: updateBlendCcolor()
        }
        Slider{
            id: aSlider
            width: 300
            minimumValue: 0
            maximumValue: 255
            stepSize: 1
            value: 255
            onValueChanged: updateBlendCcolor()
        }
    }

    Column{
        spacing: 10
        width: parent.width
        Item{
            width: row1.width
            height: 30
            Row{
                id: row1
                spacing: 5
                Repeater{
                    id: animationRep
                    model: mySpine.animations
                    Button{
                        text: modelData
                        onClicked: {
                            mySpine.setAnimation(0, modelData, true)
                        }
                    }
                }
            }
        }

        Item{
            width: row2.width
            height: 30
            Row{
                id: row2
                spacing: 5
                Repeater{
                    id: skinRep
                    model: mySpine.skins
                    Button{
                        text: modelData
                        onClicked: mySpine.setSkin(modelData)
                    }
                }
            }
        }

        Item{
            width: parent.width
            height: 20
            Row{
                CheckBox{
                    text: "debugBones"
                    onCheckedChanged: mySpine.debugBones = checked
                }

                CheckBox{
                    text: "debugSlots"
                    onCheckedChanged: mySpine.debugSlots = checked
                }
            }

        }
    }

    Column{
        spacing: 10
        anchors.right: parent.right
        Item{
            anchors.right: parent.right
            width: row3.width
            height: 30
            Row{
                id: row3
                spacing: 5
                Text{
                    text: "fps: " + String(mySpine.fps)
                }

                Slider{
                    minimumValue: 1
                    maximumValue: 60
                    stepSize: 1
                    value: 60
                    id: fpsSlider
                }
            }
        }

        Item{
            anchors.right: parent.right
            width: row4.width
            height: 30
            Row{
                id: row4
                spacing: 5
                Text{
                    text: "scale: " + String(mySpine.skeletonScale)
                }

                Slider{
                    minimumValue: 0.1
                    maximumValue: 2
                    stepSize: 0.1
                    value: 1
                    id: scaleSlider
                }
            }
        }

        Item{
            anchors.right: parent.right
            width: row5.width
            height: 30
            Row{
                id: row5
                spacing: 5
                Text{
                    text: "time scale: " + String(mySpine.timeScale)
                }

                Slider{
                    minimumValue: 0.1
                    maximumValue: 2
                    stepSize: 0.1
                    value: 1
                    id: timeScaleSlider
                }
            }
        }

        Item{
            anchors.right: parent.right
            width: row6.width
            height: 30
            Row{
                id: row6
                spacing: 5
                Text{
                    text: "default Mix: " + String(mySpine.defaultMix)
                }

                Slider{
                    minimumValue: 0.1
                    maximumValue: 2
                    stepSize: 0.1
                    value: 1
                    id: defautMixSlider
                }
            }
        }

        Item{
            anchors.right: parent.right
            width: row7.width
            height: 30
            Row{
                id: row7
                spacing: 5
                Text{
                    text: "scale x: " + String(mySpine.scaleX)
                }

                Slider{
                    minimumValue: -2
                    maximumValue: 2
                    stepSize: 0.1
                    value: 1
                    id: scaleXSlider
                    onValueChanged: mySpine.scaleX = value
                }
            }
        }

        Item{
            anchors.right: parent.right
            width: row8.width
            height: 30
            Row{
                id: row8
                spacing: 5
                Text{
                    text: "scale y: " + String(mySpine.scaleY)
                }

                Slider{
                    minimumValue: -2
                    maximumValue: 2
                    stepSize: 0.1
                    value: 1
                    id: scaleYSlider
                    onValueChanged: mySpine.scaleY = value
                }
            }
        }
    }


    FPSText {
        id: fpsText
        width: 100
        height: 30
        Text {
            anchors.centerIn: parent
            font.pixelSize: 16
            text: fpsText.fps
        }
    }
    Repeater{
        id: spReap
        model: 13
        SpineItem{
            atlasFile: "examples/alien/export/alien-pma.atlas"
            skeletonFile: "examples/alien/export/alien-ess.json"
            fps: fpsSlider.value
            skeletonScale: 0.2
            x: Math.floor(index / 20) * 60
            y: index % 20 * 60
            timeScale: timeScaleSlider.value
            defaultMix: 0.1
            onResourceReady: {
//                setAnimation(0, "death", true);
                addAnimation(0, "jump", false, 0)
                addAnimation(0, "run", false, 0)
                addAnimation(0, "hit", false, 0)
                addAnimation(0, "jump", false, 0)
                addAnimation(0, "run", false, 0)
                addAnimation(0, "hit", false, 0)
                addAnimation(0, "jump", false, 0)
                addAnimation(0, "run", false, 0)
                addAnimation(0, "hit", false, 0)
                addAnimation(0, "jump", false, 0)
                addAnimation(0, "run", false, 0)
                addAnimation(0, "hit", false, 0)
                addAnimation(0, "jump", false, 0)
                addAnimation(0, "run", false, 0)
                addAnimation(0, "hit", false, 0)
                addAnimation(0, "death", false, 0)
                addAnimation(0, "run", true, 0)
            }
        }
    }


    Slider{
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        minimumValue: 0
        maximumValue: 100
        width: 600
        id: countSlider
        onValueChanged: {
            spReap.model = value
        }
    }

}
