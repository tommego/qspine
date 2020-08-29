import QtQuick 2.12
import QtQuick.Window 2.12
import Beelab.SpineItem 1.0
import QtQuick.Controls 1.4

Window {
    visible: true
    width: 1000
    height: 700
    title: qsTr("Hello World")

    SpineItem{
        id: mySpine
        atlasFile: "examples/raptor/export/raptor-pma.atlas"
        skeletonFile: "examples/raptor/export/raptor-pro.json"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        fps: fpsSlider.value
        skeletonScale: scaleSlider.value
        timeScale: timeScaleSlider.value
        defaultMix: defautMixSlider.value
        onAnimationsChanged: {
            animationRep.model = animations
            skinRep.model = skins
        }
    }

    Column{
        spacing: 10
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

}
