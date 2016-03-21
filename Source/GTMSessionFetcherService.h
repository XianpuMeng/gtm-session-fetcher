/* Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// For best performance and convenient usage, fetchers should be generated by a common
// GTMSessionFetcherService instance, like
//
//   _fetcherService = [[GTMSessionFetcherService alloc] init];
//   GTMSessionFetcher* myFirstFetcher = [_fetcherService fetcherWithRequest:request1];
//   GTMSessionFetcher* mySecondFetcher = [_fetcherService fetcherWithRequest:request2];

#import "GTMSessionFetcher.h"

GTM_ASSUME_NONNULL_BEGIN

// Notifications.

// This notification indicates a reusable session has become invalid. It is intended mainly for the
// service's unit tests.
//
// The notification object is the fetcher service.
// The invalid session is provided via the userInfo kGTMSessionFetcherServiceSessionKey key.
extern NSString *const kGTMSessionFetcherServiceSessionBecameInvalidNotification;
extern NSString *const kGTMSessionFetcherServiceSessionKey;

@interface GTMSessionFetcherService : NSObject<GTMSessionFetcherServiceProtocol>

// Queues of delayed and running fetchers. Each dictionary contains arrays
// of GTMSessionFetcher *fetchers, keyed by NSString *host
@property(atomic, strong, readonly, GTM_NULLABLE) GTM_NSDictionaryOf(NSString *, NSArray *) *delayedFetchersByHost;
@property(atomic, strong, readonly, GTM_NULLABLE) GTM_NSDictionaryOf(NSString *, NSArray *) *runningFetchersByHost;

// A max value of 0 means no fetchers should be delayed.
// The default limit is 10 simultaneous fetchers targeting each host.
// This does not apply to fetchers whose useBackgroundSession property is YES. Since services are
// not resurrected on an app relaunch, delayed fetchers would effectively be abandoned.
@property(atomic, assign) NSUInteger maxRunningFetchersPerHost;

// Properties to be applied to each fetcher; see GTMSessionFetcher.h for descriptions
@property(atomic, strong, GTM_NULLABLE) NSURLSessionConfiguration *configuration;
@property(atomic, copy, GTM_NULLABLE) GTMSessionFetcherConfigurationBlock configurationBlock;
@property(atomic, strong, GTM_NULLABLE) NSHTTPCookieStorage *cookieStorage;
@property(atomic, strong, GTM_NULLABLE) dispatch_queue_t callbackQueue;
@property(atomic, strong, GTM_NULLABLE) NSURLCredential *credential;
@property(atomic, strong) NSURLCredential *proxyCredential;
@property(atomic, copy, GTM_NULLABLE) GTM_NSArrayOf(NSString *) *allowedInsecureSchemes;
@property(atomic, assign) BOOL allowLocalhostRequest;
@property(atomic, assign) BOOL allowInvalidServerCertificates;
@property(atomic, assign, getter=isRetryEnabled) BOOL retryEnabled;
@property(atomic, copy, GTM_NULLABLE) GTMSessionFetcherRetryBlock retryBlock;
@property(atomic, assign) NSTimeInterval maxRetryInterval;
@property(atomic, assign) NSTimeInterval minRetryInterval;
@property(atomic, copy, GTM_NULLABLE) GTM_NSDictionaryOf(NSString *, id) *properties;

// A default useragent of GTMFetcherStandardUserAgentString(nil) will be given to each fetcher
// created by this service unless the request already has a user-agent header set.
// This default will be added starting with builds with the SDKs for OS X 10.11 and iOS 9.
//
// To use the configuration's default user agent, set this property to nil.
@property(atomic, copy, GTM_NULLABLE) NSString *userAgent;

// The authorizer to attach to the created fetchers. If a specific fetcher should
// not authorize its requests, the fetcher's authorizer property may be set to nil
// before the fetch begins.
@property(atomic, strong, GTM_NULLABLE) id<GTMFetcherAuthorizationProtocol> authorizer;

// Delegate queue used by the session when calling back to the fetcher.  The default
// is the main queue.  Changing this does not affect the queue used to call back to the
// application; that is specified by the callbackQueue property above.
@property(atomic, strong, GTM_NULLABLE) NSOperationQueue *sessionDelegateQueue;

// When enabled, indicates the same session should be used by subsequent fetchers.
//
// This is enabled by default.
@property(atomic, assign) BOOL reuseSession;

// Sets the delay until an unused session is invalidated.
// The default interval is 60 seconds.
//
// If the interval is set to 0, then any reused session is not invalidated except by
// explicitly invoking -resetSession.  Be aware that setting the interval to 0 thus
// causes the session's delegate to be retained until the session is explicitly reset.
@property(atomic, assign) NSTimeInterval unusedSessionTimeout;

// If shouldReuseSession is enabled, this will force creation of a new session when future
// fetchers begin.
- (void)resetSession;

// Create a fetcher
//
// These methods will return a fetcher. If successfully created, the connection
// will hold a strong reference to it for the life of the connection as well.
// So the caller doesn't have to hold onto the fetcher explicitly unless they
// want to be able to monitor or cancel it.
- (GTMSessionFetcher *)fetcherWithRequest:(NSURLRequest *)request;
- (GTMSessionFetcher *)fetcherWithURL:(NSURL *)requestURL;
- (GTMSessionFetcher *)fetcherWithURLString:(NSString *)requestURLString;
- (id)fetcherWithRequest:(NSURLRequest *)request
            fetcherClass:(Class)fetcherClass;

- (BOOL)isDelayingFetcher:(GTMSessionFetcher *)fetcher;

- (NSUInteger)numberOfFetchers;        // running + delayed fetchers
- (NSUInteger)numberOfRunningFetchers;
- (NSUInteger)numberOfDelayedFetchers;

// Return a list of all running or delayed fetchers. This includes fetchers created
// by the service which have been started and have not yet stopped.
//
// Returns an array of fetcher objects, or nil if none.
- (GTM_NULLABLE GTM_NSArrayOf(GTMSessionFetcher *) *)issuedFetchers;

// Search for running or delayed fetchers with the specified URL.
//
// Returns an array of fetcher objects found, or nil if none found.
- (GTM_NULLABLE GTM_NSArrayOf(GTMSessionFetcher *) *)issuedFetchersWithRequestURL:(NSURL *)requestURL;

- (void)stopAllFetchers;

// Methods for use by the fetcher class only.
- (GTM_NULLABLE NSURLSession *)session;
- (GTM_NULLABLE NSURLSession *)sessionForFetcherCreation;
- (GTM_NULLABLE id<NSURLSessionDelegate>)sessionDelegate;
- (GTM_NULLABLE NSDate *)stoppedAllFetchersDate;

// The testBlock can inspect its fetcher parameter's mutableRequest property to
// determine which fetcher is being faked.
@property(copy, GTM_NULLABLE) GTMSessionFetcherTestBlock testBlock;

@end

@interface GTMSessionFetcherService (TestingSupport)

// Convenience method to create a fetcher service for testing.
//
// Fetchers generated by this mock fetcher service will not perform any
// network operation, but will invoke callbacks and provide the supplied data
// or error to the completion handler.
//
// You can make more customized mocks by setting the test block property of the service
// or fetcher; the test block can inspect the fetcher's request or other properties.
//
// See the description of the testBlock property below.
+ (instancetype)mockFetcherServiceWithFakedData:(GTM_NULLABLE NSData *)fakedDataOrNil
                                     fakedError:(GTM_NULLABLE NSError *)fakedErrorOrNil;

// Spin the run loop and discard events (or, if not on the main thread, just sleep the thread)
// until all running and delayed fetchers have completed.
//
// This is only for use in testing or in tools without a user interface.
//
// Synchronous fetches should never be done by shipping apps; they are
// sufficient reason for rejection from the app store.
//
// Returns NO if timed out.
- (BOOL)waitForCompletionOfAllFetchersWithTimeout:(NSTimeInterval)timeoutInSeconds;

@end

@interface GTMSessionFetcherService (BackwardsCompatibilityOnly)

// Clients using GTMSessionFetcher should set the cookie storage explicitly themselves.
// This method is just for compatibility with the old fetcher.
@property(atomic, assign) NSInteger cookieStorageMethod;

@end

GTM_ASSUME_NONNULL_END
