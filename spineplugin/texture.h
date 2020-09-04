/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.1
 *
 * Copyright (c) 2013, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to install, execute and perform the Spine Runtimes
 * Software (the "Software") solely for internal use. Without the written
 * permission of Esoteric Software (typically granted by licensing Spine), you
 * may not (a) modify, translate, adapt or otherwise create derivative works,
 * improvements of the Software or develop new applications using the Software
 * or (b) remove, delete, alter or obscure any trademarks or any copyright,
 * trademark, patent or other intellectual property or proprietary rights
 * notices on or in the Software, including any copy thereof. Redistributions
 * in binary or source form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <QtGlobal>
#include <QSize>
#include <QString>
#include <QSGTexture>
#include <QSharedPointer>
#include <QMutex>
#include <QQuickWindow>
#include <spine/spine.h>

class QImage;

struct Texture
{
public:
    explicit Texture(const QString& _name):name(_name){}
    QString name;
};

class AimyTextureLoader: public spine::TextureLoader{
public:
    AimyTextureLoader();
    ~AimyTextureLoader() override;
    static AimyTextureLoader* instance();
    virtual void load(spine::AtlasPage &page, const spine::String &path) override;
    virtual void unload(void *texture) override;
    QSGTexture* getGLTexture(Texture*texture, QQuickWindow*window);
    void releaseTextures();

private:
    QHash<QString, QSharedPointer<Texture>> m_textureHash;
    QHash<QString, QSGTexture*> m_glTextureHash;
    QMutex m_mutex;
};

class AimyExtension: public spine::DefaultSpineExtension{
public:
    AimyExtension();
    virtual ~AimyExtension() override;

protected:
    virtual char * _readFile(const spine::String &path, int *length) override;
};

#endif // TEXTURE_H
