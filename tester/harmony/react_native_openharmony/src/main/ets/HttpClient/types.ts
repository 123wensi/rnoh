import http from '@ohos.net.http'

export interface RequestInterceptor {
  shouldIntercept(url: string, requestOptions: RequestOptions): boolean

  intercept(url: string, requestOptions: RequestOptions): {
    url: string,
    requestOptions: RequestOptions
  }
}

export interface ResponseInterceptor {
  shouldIntercept(url: string, response: HttpResponse): boolean

  intercept(url: string, response: HttpResponse): HttpResponse

}

export interface PartialProgress {
  totalLength: number,
  lengthReceived: number,
  bitsReceived: ArrayBuffer
}

export interface HttpResponse {
  statusCode: number,
  headers: object,
  body: string | ArrayBuffer,
}

export interface HttpErrorResponse {
  statusCode: number,
  error: Error
}

export type RequestOptions = http.HttpRequestOptions

export type CancelRequestCallback = () => void
