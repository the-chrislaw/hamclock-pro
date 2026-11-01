#include "file_access.hpp"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

namespace hamclock::file_access {

bool canChooseFiles() { return true; }

QString chooseFileToRead(const QString& title) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.message = title.toNSString();
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        if ([panel runModal] != NSModalResponseOK || panel.URLs.count == 0) return {};
        return QString::fromNSString(panel.URLs.firstObject.path);
    }
}

QByteArray readBookmark(const QString& path) {
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:path.toNSString()];
        NSError* error = nil;
        NSData* data = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope |
                                                    NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess
                     includingResourceValuesForKeys:nil
                                      relativeToURL:nil
                                              error:&error];
        return data ? QByteArray::fromNSData(data) : QByteArray();
    }
}

QString openReadBookmark(const QByteArray& bookmark) {
    @autoreleasepool {
        BOOL stale = NO;
        NSError* error = nil;
        NSURL* url = [NSURL URLByResolvingBookmarkData:bookmark.toNSData()
                                               options:NSURLBookmarkResolutionWithSecurityScope
                                         relativeToURL:nil
                                   bookmarkDataIsStale:&stale
                                                 error:&error];
        if (!url) return {};
        // Held until exit (one log file). Returns NO outside the sandbox, which is fine.
        [url startAccessingSecurityScopedResource];
        return QString::fromNSString(url.path);
    }
}

} // namespace hamclock::file_access
