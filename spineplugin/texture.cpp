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

#include "texture.h"
#include <QImage>
#include <QFileInfo>
#include <QDebug>
#include <spine/Extension.h>

AimyTextureLoader::AimyTextureLoader()
{

}

AimyTextureLoader::~AimyTextureLoader()
{
    releaseTextures();
}

AimyTextureLoader *AimyTextureLoader::instance()
{
    static AimyTextureLoader _instance;
    return &_instance;
}

void AimyTextureLoader::load(spine::AtlasPage &page, const spine::String &path)
{
    QMutexLocker locker(&m_mutex);
    QString filePath(path.buffer());

    if(m_textureHash.contains(filePath)) {
        auto tex = m_textureHash.value(filePath);
        page.setRendererObject(tex.get());
        return;
    }

    if(!QFile::exists(filePath)) {
        qWarning() << filePath << " not exists...";
        return;
    }
    auto tex = QSharedPointer<Texture>(new Texture(filePath));

    if(m_window) {
        QImage img(tex->name);
        QSGTexture* glTex = m_window->createTextureFromImage(img);
        glTex->setFiltering(QSGTexture::Linear);
        glTex->setMipmapFiltering(QSGTexture::Linear);
        m_glTextureHash.insert(tex->name, glTex);
    }
    else {
        qWarning() << "window is not found " << m_window;
    }

    page.setRendererObject(tex.get());
    m_textureHash.insert(filePath, tex);
}

void AimyTextureLoader::unload(void *texture)
{
    Q_UNUSED(texture)
}

QSGTexture *AimyTextureLoader::getGLTexture(Texture *texture, QQuickWindow *window)
{
    if (!texture || texture->name.isEmpty()) {
//        qWarning() << "texture name is empty " << texture->name;
        return nullptr;
    }

    if (!window) {
//        qWarning() << "window is invalid " << window;
        return nullptr;
    }

    if (m_glTextureHash.contains(texture->name))
        return m_glTextureHash.value(texture->name);
    else
        qWarning() << "no img source found : " << texture->name;
    return nullptr;
}

void AimyTextureLoader::releaseTextures()
{
    if (m_glTextureHash.isEmpty())
        return;

    QHashIterator<QString, QSGTexture*> i(m_glTextureHash);
    while (i.hasNext()) {
        i.next();
        if (i.value())
            delete i.value();
    }

    m_glTextureHash.clear();
}

QQuickWindow *AimyTextureLoader::getWindow() const
{
    return m_window;
}

void AimyTextureLoader::setWindow(QQuickWindow *window)
{
    m_window = window;
}

AimyExtension::AimyExtension(): spine::DefaultSpineExtension()
{

}

AimyExtension::~AimyExtension()
{

}

char *AimyExtension::_readFile(const spine::String &path, int *length)
{
    QString filePath(path.buffer());
    *length = 0;
    if(!QFile::exists(filePath))
        return nullptr;
    QFile f(filePath);
    if(!f.open(QIODevice::ReadOnly)) {
        qWarning() << f.errorString();
        return nullptr;
    }
    auto bytes = f.readAll();
    f.close();
    *length = bytes.size();

    char* datas = (char*)malloc(bytes.size());
    memcpy(datas, bytes.data(), *length);

    return datas;
}

spine::SpineExtension* spine::getDefaultExtension() {
    return new AimyExtension();
}

